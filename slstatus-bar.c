#include <X11/X.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <unistd.h>

#include <sys/signal.h>
#include <errno.h>
#include <time.h>

#include "drw.h"
#include "arg.h"
#include "slstatus-bar.h"
#include "util.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

struct arg {
	const char *(*func)(const char *);
	const char *fmt;
	const char *args;
};

char buf[1024];
static volatile sig_atomic_t done;

#include "config.h"

static void
terminate(const int signo)
{
	if (signo != SIGUSR1)
		done = 1;
}

static void
difftimespec(struct timespec *res, struct timespec *a, struct timespec *b)
{
	res->tv_sec = a->tv_sec - b->tv_sec - (a->tv_nsec < b->tv_nsec);
	res->tv_nsec = a->tv_nsec - b->tv_nsec +
	               (a->tv_nsec < b->tv_nsec) * 1E9;
}

typedef struct {
    Display *dpy;
    Colormap cmap;
    Window win;
    XSetWindowAttributes swa;
    Screen* screen;
    Visual* vis;
    int depth;
    int scr;
    int x, y;
    unsigned int width, height;
} XWindow;

XWindow xw;
GC gc;
Window root;
Drw* drw;
Fnt* fnt;
Clr* scm;

unsigned int alphas[] = {0xFF, 0xF0};

void SetHostNamePid() {
    char* hostname;
    pid_t pid;
    XTextProperty txtprop;

    hostname = ecalloc(HOST_NAME_MAX, 1);
    if ((gethostname(hostname, HOST_NAME_MAX) > -1) && (pid = getpid())) {
        XStringListToTextProperty(&hostname, 1, &txtprop);
        XSetWMClientMachine(xw.dpy, xw.win, &txtprop);
        XFree(txtprop.value);

        XChangeProperty(xw.dpy, xw.win,
                        XInternAtom(xw.dpy, "_NET_WM_PID", False),
                        XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)&pid, 1);
    }
    free(hostname);
}

void SetStrut() {

    XWindowAttributes wa;

    XGetWindowAttributes(xw.dpy, xw.win, &wa);

    Atom tmp = XInternAtom(xw.dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(xw.dpy, xw.win, XInternAtom(xw.dpy, "_NET_WM_WINDOW_TYPE", False),
                    XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&tmp, 1);

    tmp = XInternAtom(xw.dpy, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(xw.dpy, xw.win, XInternAtom(xw.dpy, "_NET_WM_STATE", False),
                    XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&tmp, 1);

    tmp = XInternAtom(xw.dpy, "_NET_WM_STATE_STICKY", False);
    XChangeProperty(xw.dpy, xw.win, XInternAtom(xw.dpy, "_NET_WM_STATE", False),
                    XA_ATOM, 32, PropModeAppend,
                    (unsigned char *)&tmp, 1);

    unsigned int desktop = 0xffffffff;
    XChangeProperty(xw.dpy, xw.win, XInternAtom(xw.dpy, "_NET_WM_DESKTOP", False),
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&desktop, 1);

    long strut[12] = { 0 };
    strut[2] = wa.height;
    strut[8] = wa.x;
    strut[9] = wa.width + wa.x - 1;

    XChangeProperty(xw.dpy, xw.win, XInternAtom(xw.dpy, "_NET_WM_STRUT", False),
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&strut, 4);

    XChangeProperty(xw.dpy, xw.win, XInternAtom(xw.dpy, "_NET_WM_STRUT_PARTIAL", False),
                    XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&strut, 12);
}

void SetClassHint() {
    XClassHint* classhint;

    classhint = XAllocClassHint();
    classhint->res_name = "slstatus-bar";
    classhint->res_class = "slstatus-bar";
    XSetClassHint(xw.dpy, xw.win, classhint);
    XFree(classhint);
}

void SetWmNormalHints() {
    XSizeHints* xsh = XAllocSizeHints();

    xsh->flags = PPosition | PSize | PMinSize | PMaxSize | PBaseSize;
    xsh->x = xw.x;
    xsh->y = xw.y;
    xsh->width = xsh->base_width = xsh->min_width = xsh->max_width = xw.width;
    xsh->height = xsh->base_height = xsh->min_height = xsh->max_height = xw.height;

    XSetWMSizeHints(xw.dpy, xw.win, xsh, XA_WM_SIZE_HINTS);

    XFree(xsh);
}

void DrawString(const char *str) {
    unsigned int woffset = xw.width - drw_fontset_getwidth(drw, str) - 2;

    drw_rect(drw, xw.x, xw.y, xw.width, xw.height, 1, 1);

    drw_text(drw, 0, 0, xw.width, xw.height, woffset, str, 0);

    drw_map(drw, xw.win, xw.x, xw.y, xw.width, xw.height);
}

void xinit() {
    XWindowAttributes wa;
    XVisualInfo vis;

    xw.dpy = XOpenDisplay(NULL); if (!xw.dpy) {
        perror("Could not open display");
        exit(1);
    }
    root = DefaultRootWindow(xw.dpy);
    xw.scr = DefaultScreen(xw.dpy);
    xw.screen = ScreenOfDisplay(xw.dpy, xw.scr);

    XGetWindowAttributes(xw.dpy, root, &wa);
    xw.depth = wa.depth;

    XMatchVisualInfo(xw.dpy, xw.scr, xw.depth, TrueColor, &vis);
    xw.vis = vis.visual;

    if (!FcInit()) {
        perror("Could not init fontconfig");
        exit(1);
    }


    xw.cmap = XCreateColormap(xw.dpy, root, xw.vis, None);

    Atom ewmh = XInternAtom(xw.dpy, "_NET_SUPPORTED", False);
    Atom strut = XInternAtom(xw.dpy, "_NET_WM_STRUT", False);
    Atom strutp = XInternAtom(xw.dpy, "_NET_WM_STRUT_PARTIAL", False);
    Atom actualtype;
    long maxlength = 1024;
    int format;
    unsigned long nitems;
    unsigned long bytesleft;
    Atom* supported = NULL;
    XGetWindowProperty(xw.dpy, root, ewmh,
        0, maxlength, False, XA_ATOM, &actualtype,
        &format, &nitems, &bytesleft, (unsigned char**)&supported);
    for (unsigned long i = 0; i < nitems; i++) {
        if (supported[i] == strut || supported[i] == strutp){
            xw.swa.override_redirect = False;
            break;
        }
        else {
            xw.swa.override_redirect = True;
        }
    }

    xw.swa.background_pixmap = ParentRelative;
    xw.swa.bit_gravity = NorthWestGravity;
    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask = StructureNotifyMask;

    xw.width = WidthOfScreen(xw.screen);
    xw.height = 20;
    xw.x = xw.y = 0;

    drw = drw_create(xw.dpy, xw.scr, root, xw.width, xw.height, xw.vis, xw.depth, xw.cmap);
    fnt = drw_fontset_create(drw, fontname, 1);

    xw.height = drw->fonts->h + 2;

    xw.win = XCreateWindow(xw.dpy, root, xw.x, xw.y, xw.width, xw.height,
        0, xw.depth, CopyFromParent, xw.vis,
        CWOverrideRedirect | CWBitGravity | CWColormap | CWBackPixmap | CWEventMask, &xw.swa);

    XStoreName(xw.dpy, xw.win, "HELLO");

    SetClassHint();
    SetHostNamePid();
    SetWmNormalHints();
    SetStrut();

    scm = drw_scm_create(drw, colors, alphas, 2);
    drw_setscheme(drw, scm);
}

int main() {

	struct sigaction act;
	struct timespec start, current, diff, intspec, wait;
	size_t i, len;
	int ret;
	char status[MAXLEN];
	const char *res;

    xinit();

    XMapWindow(xw.dpy, xw.win);
    XSync(xw.dpy, False);

    XSetWindowBackground(xw.dpy, xw.win, 0x000000);
    XClearWindow(xw.dpy, xw.win);

    memset(&act, 0, sizeof(act));
	act.sa_handler = terminate;
	sigaction(SIGINT,  &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	act.sa_flags |= SA_RESTART;
	sigaction(SIGUSR1, &act, NULL);

    do {
        if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
            die("clock_gettime:");

        status[0] = '\0';
        for (i = len = 0; i < LEN(args); i++) {
            if (!(res = args[i].func(args[i].args)))
              res = unknown_str;

            if ((ret = esnprintf(status + len, sizeof(status) - len,
                                 args[i].fmt, res)) < 0)
              break;

            len += ret;
        }

        DrawString(status);
        XFlush(xw.dpy);

        if (!done) {
            if (clock_gettime(CLOCK_MONOTONIC, &current) < 0)
              die("clock_gettime:");
            difftimespec(&diff, &current, &start);

            intspec.tv_sec = interval / 1000;
            intspec.tv_nsec = (interval % 1000) * 1E6;
            difftimespec(&wait, &intspec, &diff);

            if (wait.tv_sec >= 0 && nanosleep(&wait, NULL) < 0 &&
                errno != EINTR)
              die("nanosleep:");
        }
    } while (!done);

    drw_free(drw);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);

    return 0;
}
