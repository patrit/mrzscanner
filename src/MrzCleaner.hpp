#ifndef MRZ_CLEANUP_H
#define MRZ_CLEANUP_H

#include <string_view>
#include <unordered_map>

class MrzCleaner {
public:
  MrzCleaner(std::unordered_map<char, char> spec);
  std::string fix(std::string_view str, size_t len) const;

private:
  std::unordered_map<char, char> _map;
};

#endif