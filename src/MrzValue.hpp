#ifndef MRZ_VALUE_H
#define MRZ_VALUE_H

#include <string>

class MrzValue {
  std::string value;
  char checkCalculated;
  std::string concatenated;

public:
  MrzValue() = default;
  MrzValue(std::string val)
      : value(val), checkCalculated('\0'), concatenated() {}
  ~MrzValue() = default;

  void updateCheck(char calc, std::string concat) {
    checkCalculated = calc;
    concatenated = concat;
  }
  std::string &getValue() { return value; }
  std::string const &getValue() const { return value; }
  bool isCheck() const { return !concatenated.empty(); }
  char getCheckExpected() const { return value[0]; }
  char getCheckCalculated() const { return checkCalculated; }
  std::string const &getConcatenatedString() const { return concatenated; }
  bool isValid() const { return (getCheckExpected() == '<') || (getCheckExpected() == getCheckCalculated()); }
};

#endif