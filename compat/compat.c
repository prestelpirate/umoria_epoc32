/* EPOC32 libc gaps for umoria */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* estlib has no snprintf: format into a big scratch buffer, then truncate. */
static char snbuf[8192];
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
	int len = vsprintf(snbuf, fmt, ap);
	if (n > 0) {
		size_t c = ((size_t)len < n) ? (size_t)len : n - 1;
		memcpy(buf, snbuf, c); buf[c] = 0;
	}
	return len;
}
int snprintf(char *buf, size_t n, const char *fmt, ...) {
	va_list ap; int r;
	va_start(ap, fmt); r = vsnprintf(buf, n, fmt, ap); va_end(ap);
	return r;
}
