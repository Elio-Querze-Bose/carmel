#ifndef GRAEHL_SHARED__HAVE_64_BITS_HPP
#define GRAEHL_SHARED__HAVE_64_BITS_HPP

#include <stdint.h>

// pointer at least 64 bits if 1, exactly 32bits if 0
#if !defined(HAVE_64_BITS)
# if INTPTR_MAX == INT32_MAX
#  define HAVE_64_BITS 0
# elif INTPTR_MAX >= INT64_MAX
#  define HAVE_64_BITS 1
# else
#  error "couldn't tell if HAVE_64_BITS from INTPTR_MAX INT32_MAX INT64_MAX"
# endif
#endif

// long is bigger than int
#if !defined(HAVE_LONGER_LONG)
# if defined(INT_MAX) && defined(LONG_MAX)
#  if LONG_MAX!=INT_MAX
#   define HAVE_LONGER_LONG 1
#  else
#   define HAVE_LONGER_LONG 0
#  endif
# else
#  define HAVE_LONGER_LONG 0
# endif
#endif


#endif