/*
 * Copyright ©2020 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to the students registered for University of Washington
 * CSE 333 for use solely during Fall Quarter 2020 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>

extern "C" {
  #include "libhw2/FileParser.h"
}

#include "./HttpUtils.h"
#include "./FileReader.h"

using std::string;

namespace hw4 {

bool FileReader::ReadFile(string *str) {
  string fullfile = basedir_ + "/" + fname_;

  // Read the file into memory, and store the file contents in the
  // output parameter "str."  Be careful to handle binary data
  // correctly; i.e., you probably want to use the two-argument
  // constructor to std::string (the one that includes a length as a
  // second argument).
  //
  // You might find ::ReadFileToString() from HW2 useful
  // here.  Be careful, though; remember that it uses malloc to
  // allocate memory, so you'll need to use free() to free up that
  // memory.  Alternatively, you can use a unique_ptr with a malloc/free
  // deleter to automatically manage this for you; see the comment in
  // HttpUtils.h above the MallocDeleter class for details.

  // STEP 1:
  // if the path is not safe, the fullfile is not in the directory
  // print out the error message
  if (!IsPathSafe(basedir_, fullfile)) {
    std::cerr << "the full file path is not correct" << std::endl;
    return false;
  }
  int size;
  char* file_content;
  // Reads the full contents of "filename" into memory, malloc'ing space for
  // its contents and returning a pointer to the allocated memory.
  file_content = ::ReadFileToString(fullfile.c_str(), &size);
  if (file_content == nullptr) {
    return false;
  }
  // store the file content in format string
  *str = string(file_content, size);
  free(file_content);
  return true;
}

}  // namespace hw4
