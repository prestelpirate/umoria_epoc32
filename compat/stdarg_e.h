#ifndef EPOC_COMPAT_STDARG_E_H
#define EPOC_COMPAT_STDARG_E_H
#include_next <stdarg_e.h>
/* estlib's va_start is &lastarg + sizeof(lastarg). That is wrong when the
 * last named argument is narrower than int (e.g. bool) or when gcc spills the
 * register argument to a local slot: ap ends up misaligned / pointing at the
 * wrong place -> KERN-EXEC 3 (alignment fault) on StrongARM. Use gcc's
 * __builtin_next_arg, which always gives the first anonymous argument. */
#undef va_start
#define va_start(ap, pn) ((ap) = (__e32_va_list)__builtin_next_arg(pn), (void)0)
/* ARM APCS va_list is a plain pointer, so assignment copies it */
#ifndef va_copy
#define va_copy(d, s) ((d) = (s))
#endif
#endif
