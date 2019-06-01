#include "MrzValid.hpp"

using namespace std;

uint8_t MrzValid::val(char c) {
  if (('0' <= c) && (c <= '9')) {
    return c - '0';
  }
  if (('A' <= c) && (c <= 'Z')) {
    return c - 'A' + 10;
  }
  return 0; // remaining: '<'
}

char MrzValid::calc(string const &s) {
  uint8_t weight[] = {7, 3, 1};
  size_t idx = 0;
  uint32_t sum = 0;
  for (char c : s) {
    sum += val(c) * weight[idx];
    idx = (idx + 1) % 3;
  }
  return '0' + (sum % 10);
}
