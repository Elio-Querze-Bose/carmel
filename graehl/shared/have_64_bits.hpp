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

    detect: pointer and size_t are 64 bits
*/

#ifndef GRAEHL_SHARED__HAVE_64_BITS_HPP
#define GRAEHL_SHARED__HAVE_64_BITS_HPP
#pragma once

#ifndef HAVE_64_BITS

// Check windows
#if defined(_WIN32) || defined(_WIN64)
#if defined(_WIN64)
#define HAVE_64_BITS 1
#else
#define HAVE_64_BITS 0
#endif
#elif __x86_64__ || __ppc64__
#define HAVE_64_BITS 1
#else
#define HAVE_64_BITS 0
#endif

#endif  // HAVE_64_BITS

#endif
