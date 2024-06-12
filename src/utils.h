#pragma once

#include <Rcpp.h>

using namespace Rcpp;

// Simple parser: does not check that order of numbers and letters is correct
// std::pair<int, int> parseRef(std::string ref) {
//   int col = 0, row = 0;

//   for (size_t i = 0; i < ref.size(); ++i) {
//     if (ref[i] >= '0' && ref[i] <= '9') {
//       row = row * 10 + (ref[i] - '0');
//     } else if (ref[i] >= 'A' && ref[i] <= 'Z') {
//       col = 26 * col + (ref[i] - 'A' + 1);
//     } else {
//       stop("Invalid character '%s' in cell ref '%s'", ref[i], ref);
//     }
//   }

//   return std::make_pair(row - 1, col - 1); // zero indexed
// }
std::pair<int, int> parseRef(const char* ref) {
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
