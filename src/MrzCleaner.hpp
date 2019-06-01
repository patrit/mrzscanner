#include <map>
#include <string_view>

class MrzCleaner {
public:
  MrzCleaner(std::map<char, char> spec);
  std::string fix(std::string_view str, size_t len) const;

private:
  std::map<char, char> _map;
};
