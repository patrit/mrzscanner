#ifndef MRZ_CLEANUP_H
#define MRZ_CLEANUP_H

#include <string_view>
#include <unordered_map>

class MrzCleaner {
public:
  using CharMap = std::unordered_map<char, char>;

  MrzCleaner(CharMap const &spec);
  std::string fix(std::string_view str) const;
  static void fixNumber(std::string &str, char checksum);

  static CharMap none;
  static CharMap alpha;
  static CharMap numeric;

private:
  CharMap const &_map;
};

#endif