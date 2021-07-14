#include "MrzFields.hpp"

#include <unordered_map>

using namespace std;

namespace {
std::unordered_map<Field, std::string> _map = {
    {MrzType, "mrz_type"},
    {Type, "type"},
    {Country, "country"},
    {NameCombined, ""},
    {Surname, "surname"},
    {Names, "names"},
    {Number, "number"},
    {CheckNumber, "check_number"},
    {Nationality, "nationality"},
    {DateOfBirth, "date_of_birth"},
    {CheckDateOfBirth, "check_date_of_birth"},
    {Sex, "sex"},
    {CreationDate, "creation_date"},
    {ExpirationDate, "expiration_date"},
    {CheckExpirationDate, "check_expiration_date"},
    {Optional1, "optional1"},
    {Optional2, "optional2"},
    {PersonalNumber, "personal_number"},
    {CheckPersonalNumber, "check_personal_number"},
    {CheckCompLine2_td2, "check_composed_line2"},
    {CheckCompLine2_td3, "check_composed_line2"},
    {CheckCompLine12, "check_composed_line12"},
    {CheckCompLine2_fra, "check_composed_line2"},
    {CheckCompLine12_fra, "check_composed_line12_fra"},
    {Raw1, ""},
    {Raw2, ""},
    {Raw3, ""},
};
};

unordered_map<Field, string> const &MrzFields::getMap() {
  return _map;
}

string const &MrzFields::toName(Field field) {
  static std::string _empty;
  auto it = _map.find(field);
  if (it == _map.end()) {
    return _empty;
  }
  return it->second;
}

string MrzFields::removeTrailingChar(string const& val) {
  size_t idx = val.find_last_not_of('<');
  if (idx != string::npos) {
    return val.substr(0, idx + 1);
  }
  return string();
}
