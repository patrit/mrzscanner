#include "MrzCleaner.hpp"
#include "MrzValid.hpp"

using namespace std;

MrzCleaner::CharMap MrzCleaner::none = {};
MrzCleaner::CharMap MrzCleaner::alpha = {{'0', 'O'}, {'1', 'I'}, {'2', 'Z'},
                                         {'4', 'A'}, {'5', 'S'}, {'6', 'G'},
                                         {'8', 'B'}};
MrzCleaner::CharMap MrzCleaner::numeric = {{'B', '8'}, {'C', '0'}, {'D', '0'},
                                           {'G', '6'}, {'I', '1'}, {'O', '0'},
                                           {'Q', '0'}, {'S', '5'}, {'Z', '2'}};

MrzCleaner::MrzCleaner(CharMap const &spec) : _map(spec) {}

string MrzCleaner::fix(string_view str) const {
  string ret;
  for (char c : str) {
    auto it = _map.find(c);
    if (it != _map.end()) {
      ret += it->second;
    } else {
      ret += c;
    }
  }
  return ret;
};

void MrzCleaner::fixNumber(string &str, char checksum) {
  if (str.empty() || (MrzValid::calc(str) == checksum)) {
    return;
  }
  string number(str);
  // typically number at the end -> reverse iteration
  for (int i = number.size() - 1; i >= 0; i--) {
    auto it = numeric.find(number[i]);
    if (it != numeric.end()) {
      number[i] = it->second;
    }
    if (MrzValid::calc(number) == checksum) {
      str = number;
      return;
    }
  }
  // first 2 chars often alphanum
  for (size_t i = 0; i < 2; i++) {
    auto it = alpha.find(number[i]);
    if (it != alpha.end()) {
      number[i] = it->second;
    }
    if (MrzValid::calc(number) == checksum) {
      str = number;
      return;
    }
  }
};
