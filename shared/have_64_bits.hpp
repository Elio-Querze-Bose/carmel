/** \file

    detect: pointer and size_t are 64 bits
*/

#ifndef GRAEHL_SHARED__HAVE_64_BITS_HPP
#define GRAEHL_SHARED__HAVE_64_BITS_HPP
#pragma once

#ifndef HAVE_64_BITS

// Check windows
#if defined(_WIN32) || defined(_WIN64)
# if defined(_WIN64)
#  define HAVE_64_BITS 1
# else
#  define HAVE_64_BITS 0
# endif
#elif __x86_64__ || __ppc64__
# define HAVE_64_BITS 1
#else
# define HAVE_64_BITS 0
#endif

#endif // HAVE_64_BITS

#endif
