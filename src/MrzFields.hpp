#ifndef MRZ_FIELDS_H
#define MRZ_FIELDS_H

#include <string>
#include <unordered_map>

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
  CreationDate,
  ExpirationDate,
  CheckExpirationDate,
  Optional1,
  Optional2,
  PersonalNumber,
  CheckPersonalNumber,
  CheckCompLine2_td2,
  CheckCompLine2_td3,
  CheckCompLine12,
  CheckCompLine2_fra,
  CheckCompLine12_fra,
  Raw1,
  Raw2,
  Raw3
};

class MrzFields {
public:
  MrzFields() = delete;

  static std::unordered_map<Field, std::string> const &getMap();
  static std::string const &toName(Field field);
  static std::string removeTrailingChar(std::string const& val);
};

#endif