/** \file

    for swapping objects of the same concrete type where just swapping their
    bytes will work.  will at least work on plain old data. memcpy is usually a
    fast intrinsic optimized by the compiler
*/

#ifndef GRAEHL_SHARED__SWAP_POD
#define GRAEHL_SHARED__SWAP_POD
#pragma once

namespace graehl {

#include <algorithm>  // not used, but people who use this will want to bring std::swap in anyway
#include <cstring>

template <class T>
inline void swap_pod(T& a, T& b) {
  using namespace std;
  unsigned const s = sizeof(T);
  char tmp[s];
  void* pt = (void*)tmp;
  void* pa = (void*)&a;
  void* pb = (void*)&b;
  memcpy(pt, pa, s);
  memcpy(pa, pb, s);
  memcpy(pb, pt, s);
}


}

#endif
