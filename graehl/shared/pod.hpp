// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    POD (little-endian on intel/amd) byte encoding/decoding (memcpy) and (fixed
    or serialized/variable size) array POD

    note argument order:

    decode(value, const_byteptr)

    encode(byteptr, value)

    with two-byteptr range [begin,end) you get an encoding_error if end reached
*/


#ifndef GRAEHL_SHARED__POD_HPP
#define GRAEHL_SHARED__POD_HPP
#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

namespace graehl {

struct encoding_error : std::exception {
  char const* what() const throw() { return "binary encoding didn't fit in buffer"; }
  ~encoding_error() throw() {}
};

typedef unsigned char byte;
typedef byte const* const_byteptr;
typedef byte* byteptr;

template <class Vec>
void append_array(Vec& vec, typename Vec::value_type const* data, typename Vec::value_type const* end) {
  vec.insert(vec.end(), data, end);
}

template <class Vec>
void append_array(Vec& vec, typename Vec::value_type const* data, std::size_t n) {
  append_array(vec, data, data + n);
}

template <class Vec>
void set_array(Vec& vec, typename Vec::value_type const* data, typename Vec::value_type const* end) {
  vec.~Vec();
  new (&vec) Vec(data, end);
}

template <class Vec>
void set_array(Vec& vec, typename Vec::value_type const* data, std::size_t n) {
  set_array(vec, data, data + n);
}

inline void set_array(std::string& str, char const* i, char const* end) {
  str.assign(i, end);
}

inline void set_array(std::string& str, char const* i, std::string::size_type len) {
  str.assign(i, i + len);
}

typedef char const* charptr;

struct pod : std::pair<charptr, charptr> {
  template <class Pod>
  static const_byteptr decode(Pod& x, const_byteptr p) {
    x = *(Pod*)p;
    return p + sizeof(Pod);
  }
  template <class Pod>
  static const_byteptr decode(Pod& x, const_byteptr p, const_byteptr end) {
    if (p + sizeof(Pod) > end) throw encoding_error();
    x = *(Pod*)p;
    return p + sizeof(Pod);
  }

  template <class Pod>
  static void decode(Pod& x, std::string const& str) {
    if (str.size() != sizeof(Pod)) throw encoding_error();
    x = *(Pod*)&str[0];
  }
  template <class Pod>
  static Pod decoded(std::string const& str) {
    if (str.size() != sizeof(Pod)) throw encoding_error();
    return *(Pod*)&str[0];
  }

  template <class Pod>
  static byteptr encode(byteptr p, Pod x) {
    *(Pod*)p = x;
    return p + sizeof(Pod);
  }
  template <class Pod>
  static byteptr encode(byteptr p, const_byteptr end, Pod x) {
    if (p + sizeof(Pod) > end) throw encoding_error();
    *(Pod*)p = x;
    return p + sizeof(Pod);
  }

  template <class Pod>
  static const_byteptr decode_array(Pod* x, std::size_t n, const_byteptr p) {
    n *= sizeof(Pod);
    std::memcpy(x, p, n);
    return p + n;
  }
  template <class Pod>
  static const_byteptr decode_array(Pod* x, std::size_t n, const_byteptr p, const_byteptr end) {
    n *= sizeof(Pod);
    if (p + n > end) throw encoding_error();
    std::memcpy(x, p, n);
    return p + n;
  }

  template <class PodVec>
  static const_byteptr decode_resize_array(PodVec& x, std::size_t n, const_byteptr p) {
    n *= sizeof(x[0]);
    x.resize(n);
    std::memcpy(&x[0], p, n);
    return p + n;
  }
  template <class PodVec>
  static const_byteptr decode_resize_array(PodVec& x, std::size_t n, const_byteptr p, const_byteptr end) {
    n *= sizeof(x[0]);
    if (p + n > end) throw encoding_error();
    x.resize(n);
    std::memcpy(&x[0], p, n);
    return p + n;
  }

  template <class PodVec>
  static const_byteptr decode_reconstruct_array(PodVec& x, std::size_t n, const_byteptr p) {
    typedef typename PodVec::value_type const* P;
    P data = (P)p, endp = data + n;
    set_array(x, data, endp);
    return (const_byteptr)endp;
  }

  template <class PodVec>
  static const_byteptr decode_reconstruct_array(PodVec& x, std::size_t n, const_byteptr p, const_byteptr end) {
    typedef typename PodVec::value_type const* P;
    P data = (P)p, endp = data + n;
    if ((const_byteptr)endp > end) throw encoding_error();
    set_array(x, data, endp);
    return (const_byteptr)endp;
  }

  template <class PodVec>
  static void decode_reconstruct_array(PodVec& x, const_byteptr p, const_byteptr end) {
    typedef typename PodVec::value_type V;
    assert((end - p) % sizeof(V) == 0);
    set_array(x, (V const*)p, (V const*)end);
  }

  template <class Pod>
  static byteptr encode_array(byteptr p, Pod const* x, std::size_t n) {
    n *= sizeof(Pod);
    std::memcpy(p, x, n);
    return p + n;
  }
  template <class Pod>
  static byteptr encode_array(byteptr p, const_byteptr end, Pod const* x, std::size_t n) {
    n *= sizeof(Pod);
    if (p + n > end) throw encoding_error();
    std::memcpy(p, x, n);
    return p + n;
  }
  template <class Size, class Pod>
  static std::size_t encode_size(Size sz) {
    return sizeof(Size) + sz * sizeof(Pod);
  }
  template <class Size, class Pod>
  static byteptr encode_sized(byteptr i, Pod const* x, Size sz) {
    return encode_array(encode(i, sz), x, sz);
  }
  template <class Size, class Vec>
  static const_byteptr decode_sized(Vec& vec, const_byteptr i, const_byteptr end) {
    Size sz;
    i = decode(sz, i, end);  // necessary sequence point (sets sz)
    return decode_reconstruct_array(vec, sz, i, end);
  }
  template <class Pod>
  static void append_zero(std::string& buf) {
    buf.append(sizeof(Pod), (char)0);
  }
  template <class Pod>
  static void append(std::string& buf, Pod const& x) {
    buf.append(reinterpret_cast<charptr>(&x), sizeof(Pod));
  }
  template <class Pod>
  static void append_array(std::string& buf, Pod const* x, std::size_t n) {
    buf.append((charptr)x, n * sizeof(Pod));
  }
  template <class PodVec>
  static void append_array(std::string& buf, PodVec const& x) {
    append_array(buf, &x[0], x.size());
  }
  template <class Size, class Pod>
  static void append_sized(std::string& buf, Pod const* x, Size n) {
    append(buf, n);
    append_array(buf, x, n);
  }
  template <class Size, class PodVec>
  static void append_sized(std::string& buf, PodVec const& x) {
    append_sized<Size>(buf, &x[0], (Size)x.size());
  }

  pod() : std::pair<charptr, charptr>() {}

  template <class Str>
  pod(Str const& str) {
    first = str.data();
    second = first + str.size();
  }

  template <class Str>
  pod(Str const& str, unsigned beginIndex) {
    charptr s = str.data();
    first = s + beginIndex;
    second = s + str.size();
    assert(first <= second);
  }

  char getc() {
    if (first >= second) throw encoding_error();
    return *first++;
  }

  template <class Pod>
  void get(Pod& r) {
    first = (charptr)pod::decode(r, (const_byteptr)first, (const_byteptr)second);
  }

  template <class Size, class String>
  void getSized(String& r) {
    first = (charptr)decode_sized<Size>(r, (const_byteptr)first, (const_byteptr)second);
  }

  template <class Pod>
  void skip() {
    skip(sizeof(Pod));
  }

  template <class Pod>
  Pod const* skipArray(std::size_t n) {
    Pod const* r = (Pod const*)first;
    skip(sizeof(Pod) * n);
    return r;
  }

  void skip(std::size_t n) {
    first += n;
    if (first > second) throw encoding_error();
  }

  template <class Pod>
  Pod get() {
    Pod r;
    first = (charptr)pod::decode(r, (const_byteptr)first, (const_byteptr)second);
    return r;
  }

  void get(void* dest, std::size_t size) {
    void const* from = first;
    first += size;
    if (first > second) throw encoding_error();
    std::memcpy(dest, from, size);
  }

  std::size_t size() const { return second - first; }

  template <class Pod>
  unsigned remaining() const {
    return (second - first) / sizeof(Pod);
  }

  operator bool() const {
    assert(first <= second);
    return first < second;
  }

  void set(charptr buf, charptr end) {
    first = buf;
    second = end;
  }

  // caller frees
  char* malloc(std::size_t n) {
    first = (charptr)std::malloc(n);
    second = first + n;
    return (char*)first;
  }

  charptr begin() const { return first; }
  charptr end() const { return second; }
  std::size_t bytes_decoded(charptr started) const { return first - started; }
  std::size_t bytes_remaining() const { return second - first; }
};


template <class Uint>
struct identity_codec {
  typedef Uint value_type;
  enum { k_max_bytes = sizeof(Uint) };
  static const_byteptr decode(Uint& x, const_byteptr p) {
    x = *(Uint*)p;
    return p + sizeof(Uint);
  }
  static const_byteptr decode(Uint& x, const_byteptr p, const_byteptr end) {
    if (p + sizeof(Uint) > end) throw encoding_error();
    x = *(Uint*)p;
    return p + sizeof(Uint);
  }
  static const_byteptr ignore(const_byteptr p, const_byteptr end) {
    if (p + sizeof(Uint) > end) throw encoding_error();
    return p + sizeof(Uint);
  }
  static const_byteptr ignore(const_byteptr p) { return p + sizeof(Uint); }
  static byteptr encode(byteptr p, Uint x) {
    *(Uint*)p = x;
    return p + sizeof(Uint);
  }
  static byteptr encode(byteptr p, const_byteptr end, Uint x) {
    if (p + sizeof(Uint) > end) throw encoding_error();
    *(Uint*)p = x;
    return p + sizeof(Uint);
  }
};


}

#endif
