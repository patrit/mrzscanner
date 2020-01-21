#include "MrzCleaner.hpp"

using namespace std;

MrzCleaner::MrzCleaner(unordered_map<char, char> spec) : _map(spec) {}

string MrzCleaner::fix(string_view str, size_t len) const {
  string ret;
  for (char c : str) {
    auto it = _map.find(c);
    if (it != _map.end()) {
      ret += it->second;
    } else {
      ret += c;
    }
  }
  if (ret.size() < len) {
    ret.insert(ret.end(), len - ret.size(), '<');
  }
  return ret;
};
