/* See LICENSE file for copyright and license details. */
#include <stdint.h>

extern char buf[1024];

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))
#define LEN(x) (sizeof(x) / sizeof((x)[0]))

void warn(const char *, ...);
void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);

int esnprintf(char *str, size_t size, const char *fmt, ...);
const char *bprintf(const char *fmt, ...);
const char *fmt_human(uintmax_t num, int base);
int pscanf(const char *path, const char *fmt, ...);
