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

    stub implementation of the configure protocol (see configure.hpp) that ignores
    all operations except .init(val) (and .flag(initval) which is also an init)

    this allows calling configure_init(this) for objects with

    template <class Config>
    void configure(Config &c) {
    c("val", &val).init(1);
    }

    so that this->val = 1

    this does *not* call validate (though that would be a simple addition - a
    mutable flag in init_expr would do it (have to wait until destructor so
    children are all init first)
*/

#ifndef CONFIGURE_INIT_JG_2013_05_23_HPP
#define CONFIGURE_INIT_JG_2013_05_23_HPP
#pragma once

#include <string>
#include <vector>

namespace configure {

template <class Val>
struct init_expr {
  template <class Child>
  init_expr<Child> operator()(std::string const& name, Child* child) const {
    return init_expr<Child>(child);
  }
  template <class V2>
  init_expr const& init(V2 const& v2) const {
    *val = v2;
    return *this;
  }
  template <class V2>
  init_expr const& init(bool enable, V2 const& v2) const {
    return init(v2);
  }
  // similar concept to implicit except that you have the --key implicit true, and --no-key implicit false
  init_expr const& flag(bool init_to = false) const { return init(init_to); }
  template <class V2>
  init_expr const& inits(V2 const& v2) {
    *val = v2;
    return *this;
  }

  init_expr(Val* val) : val(val) {}

  Val* val;

  /// the rest of the protocol are all no-ops

  init_expr const& null_ok(Val const& val = Val()) const { return *this; }
  init_expr const& alias(bool = true) const { return *this; }
  init_expr const& is(std::string const& is) const { return *this; }
  init_expr const& is_also(std::string const& is) const { return *this; }
  template <class V2>
  init_expr const& eg(V2 const& eg) const {
    return *this;
  }
  init_expr const& operator()(char charname) const { return *this; }
  init_expr const& operator()(std::string const& usage) const { return *this; }
  init_expr const& operator()(std::vector<char> const& usage) const { return *this; }
  init_expr const& deprecate(std::string const& info = "", bool deprecated = true) const { return *this; }
  init_expr const& init_default(bool enable = true) const { return *this; }
  init_expr const& todo(bool enable = true) const { return *this; }
  init_expr const& verbose(int verbosity = 1) const { return *this; }
  init_expr const& positional(bool enable = true, int max = 1) const { return *this; }
  template <class unrecognized_opts>
  init_expr const& allow_unrecognized(bool enable = true, bool warn = false,
                                      unrecognized_opts* unrecognized_storage = 0) const {
    return *this;
  }
  init_expr const& require(bool enable = true, bool just_warn = false) const { return *this; }
  init_expr const& desire(bool enable = true) const { return *this; }
  template <class V2>
  init_expr const& implicit(bool enable, V2 const& v2) const {
    return *this;
  }
  template <class V2>
  init_expr const& implicit(V2 const& v2) const {
    return *this;
  }
  init_expr const& defaulted(bool enable = true) const { return *this; }
  template <class V2>
  init_expr const& validate(V2 const& validator) const {
    return *this;
  }
};

template <class Val>
void configure_init(Val* val) {
  init_expr<Val> config(val);
  val->configure(config);
}


}

#endif
