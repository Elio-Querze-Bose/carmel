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
/** \file

    store in memory only the file positions necessary to quickly extract (by
    fseek+read) line #N from a large file. probably should use mmap instead of
    seek+read if speed matters.
*/

#ifndef FILELINES_HPP
#define FILELINES_HPP


#include <iostream>
#include <fstream>
#include <limits>
#include <graehl/shared/dynamic_array.hpp>
#include <graehl/shared/myassert.h>
#include <string>
#include <graehl/shared/fileargs.hpp>
#include <boost/lexical_cast.hpp>

#ifdef WINDOWS
#undef max
#endif

namespace graehl {

typedef boost::shared_ptr<std::ifstream> InSeekFile;

struct FileLines {
  static const char linesep = '\n';
  InDiskfile file;
  typedef std::streampos pos;
  dynamic_array<pos> line_begins;  // line i ranges from [line_begins[i],line_begins[i+1])
  FileLines(std::string filename) { load(filename); }
  FileLines(InDiskfile _file) : file(_file) {
    if (file) index();
  }
  void index() {
    file->seekg(0);
    line_begins.clear();
    line_begins.push_back(file->tellg());
    while (!file->eof()) {
      file->ignore(std::numeric_limits<std::streamsize>::max(), linesep);  // FIXME: does setting max unsigned
      // really work here?  not if you
      // have very very long lines ;)
      // but those don't fit into memory.
      pos end = file->tellg();
      if (end > line_begins.back()) line_begins.push_back(end);
    }
    //        should never be needed!
    Assert(file->tellg() == line_begins.back());
    /*        pos eof=file->tellg();
              if (eof > line_begins.back()) { // only necessary to handle lack of trailing newline in a
       friendly way
              line_begins.push_back(eof);
              }*/

    file->clear();  // don't want EOF flag stopping reads.
    line_begins.compact();
  }
  void load(std::string filename) {
    file.reset(new std::ifstream);
    file->open(filename.c_str(), std::ios::in | std::ios::binary);
    index();
  }
  // 0-indexed, of course
  std::string getline(unsigned i, bool chop_newline = true) {
    if (!file) {
      return boost::lexical_cast<std::string>(i);
    }
    Assert(i + 1 < line_begins.size());
    pos start = line_begins[i];
    pos end = line_begins[i + 1];
    unsigned len = end - start;
    if (chop_newline && len > 0) --len;  // don't want to include newline char
    auto_array<char> buf(len);  // FIXME: could just return a shared_ptr or use string = getline(blah,sep) ...
    // or return expression object that can convert to string or be printed
    file->seekg(start);
    file->read(buf.begin(), len);
    return std::string(buf.begin(), len);
  }
  std::string operator[](unsigned i) { return getline(i); }
  bool exists() const { return (bool)file; }
  unsigned size() const {
    Assert(exists() && line_begins.size() > 0);
    return line_begins.size() - 1;
  }
};


}

#endif
