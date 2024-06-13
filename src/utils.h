#pragma once

#include <Rcpp.h>

using namespace Rcpp;


// col = 1 --> first column aka column A, so 1-indexed
inline std::string intToABC(int col) {
  std::string ret;
  while (col > 0) {
    col--;
    ret = (char)('A' + col % 26) + ret;
    col /= 26;
  }
  return ret;
}

// row = 1, col = 1 --> upper left cell aka column A1, so 1-indexed
inline std::string asA1(const int row, const int col) {
  std::ostringstream ret;
  ret << intToABC(col) << row;
  return ret.str();
}

// Simple parser: does not check that order of numbers and letters is correct
inline std::pair<int, int> parseRef(const char* ref) {
  int col = 0, row = 0;

  for (const char* cur = ref; *cur != '\0'; ++cur) {
    if (*cur >= '0' && *cur <= '9') {
      row = row * 10 + (*cur - '0');
    } else if (*cur >= 'A' && *cur <= 'Z') {
      col = 26 * col + (*cur - 'A' + 1);
    } else {
      stop("Invalid character '%s' in cell ref '%s'", *cur, ref);
    }
  }

  return std::make_pair(row - 1, col - 1); // zero indexed
}
