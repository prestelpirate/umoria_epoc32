/* Forced-include prelude for building umoria 5.7 as C++98 (gcc 3.0 / EPOC).
 * Sources have had their std:: prefixes removed (tools/cxx98.py). */
#ifndef UMORIA_PRELUDE_H
#define UMORIA_PRELUDE_H
#define nullptr 0
#define constexpr const
#define override
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#ifdef __EPOC32__
# include <sys/types.h>
# ifndef _SSIZE_T_DEFINED
typedef int ssize_t;
# endif
# include "estring.h"         /* minimal string / ostringstream / to_string */
#else
# include <string>
# include <sstream>
# include <iostream>
using std::string;
using std::ostringstream;
using std::cerr;
using std::endl;
static inline string to_string(long v) { char b[24]; sprintf(b, "%ld", v); return string(b); }
#endif
#endif
