#ifndef MRZ_MRZ_H
#define MRZ_MRZ_H

#include <map>
#include <tuple>
#include <vector>

#include "MrzCleaner.hpp"
#include "MrzFields.hpp"
#include "MrzValue.hpp"

struct MrzField {
  Field field;
  int line;
  int start;
  int length;
  MrzCleaner const &cleaner;
};

struct MrzItem {
  std::string name;
  size_t length;
  std::vector<MrzField> fields;
};

class Mrz {
public:
  Mrz(std::string const &lines, bool debug);

  bool detected() const { return !_values.empty(); }
  std::string const &val(Field field) const;
  std::string toJSON() const;
  void toJSON(std::ostream &os) const;
  std::string const &getDebugString() const { return _debugstr; }

private:
  std::string const &_datastring;
  bool _debug;
  std::string _debugstr;
  std::map<Field, MrzValue> _values;

  std::tuple<uint8_t, uint8_t> validate();
  std::string concatCheckString(Field check,
                                std::vector<Field> const &fields) const;
  void postprocessNames();
  MrzItem const &guessType(std::vector<std::string> const &lines);
};

#endif