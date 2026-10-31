#ifndef EPOC_COMPAT_STDINT_H
#define EPOC_COMPAT_STDINT_H
typedef signed char int8_t; typedef unsigned char uint8_t;
typedef short int16_t; typedef unsigned short uint16_t;
typedef int int32_t; typedef unsigned int uint32_t;
typedef long long int64_t; typedef unsigned long long uint64_t;
#define _UINTPTR_T_DEFINED
typedef int intptr_t; typedef unsigned int uintptr_t;
typedef long long intmax_t; typedef unsigned long long uintmax_t;
#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT32_MIN (-2147483647-1)
#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295U
#define SIZE_MAX UINT32_MAX
#endif
