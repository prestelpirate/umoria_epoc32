#ifndef EPOC_COMPAT_STDDEF_H
#define EPOC_COMPAT_STDDEF_H
/* Replacement for estlib stddef.h: wchar_t must match gcc's L"" literals (16-bit) */
#ifndef NULL
#define NULL 0
#endif
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef int ptrdiff_t;
#endif
#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif
#define offsetof(T,m) ((size_t)&(((T*)0)->m))
#endif
