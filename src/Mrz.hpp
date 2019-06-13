#include <Poco/JSON/Object.h>
#include <map>
#include <vector>

#include "MrzCleaner.hpp"

enum Field {
  MrzType,
  Type,
  Country,
  NameCombined,
  Surname,
  Names,
  Number,
  CheckNumber,
  Nationality,
  DateOfBirth,
  CheckDateOfBirth,
  Sex,
  ExpirationDate,
  CheckExpirationDate,
  Optional1,
  Optional2,
  PersonalNumber,
  CheckPersonalNumber,
  CheckCompLine2,
  CheckCompLine12,
  Raw1,
  Raw2,
  Raw3
};

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
  Mrz(std::string const &lines);

  std::string toJSON() const;
  void toJSON(std::ostream &os) const;
  bool valid() const { return _valid; }

private:
  std::map<Field, std::string> _values;
  bool _valid;

  void set(Poco::JSON::Object &sum, std::string const &key, Field field) const;
  std::string const &val(Field field) const;
  bool exists(Field field) const;
  bool validate() const;
  bool isValid(Field check, std::initializer_list<Field> fields) const;
  void postprocessNames();
  MrzItem const &guessType(std::vector<std::string> const &lines);
  std::string rawString() const;
};
