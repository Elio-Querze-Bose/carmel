// Copyright 2014 Jonathan Graehl - http://graehl.org/
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
#ifndef GRAEHL_SHARED__DUMMY_HPP
#define GRAEHL_SHARED__DUMMY_HPP

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <class C>
struct dummy {
  static const C &var();
};


template <class C>
const C& dummy<C>::var() {
  static C var;
  return var;
}

#ifdef GRAEHL_TEST

BOOST_AUTO_TEST_CASE( TEST_dummy )
{
  BOOST_CHECK(dummy<int>::var() == 0);
}
#endif

}

#endif