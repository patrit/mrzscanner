#include "Mrz.hpp"

#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <algorithm>
#include <initializer_list>
#include <map>
#include <string_view>
#include <vector>

#include "MrzValid.hpp"

using namespace std;

namespace {

MrzCleaner fixNone({});
MrzCleaner fixAlpha({{'0', 'O'},
                     {'1', 'I'},
                     {'2', 'Z'},
                     {'4', 'A'},
                     {'5', 'S'},
                     {'6', 'G'},
                     {'8', 'B'}});
MrzCleaner fixNumeric({{'B', '8'},
                       {'C', '0'},
                       {'D', '0'},
                       {'G', '6'},
                       {'I', '1'},
                       {'O', '0'},
                       {'Q', '0'},
                       {'S', '5'},
                       {'Z', '2'}});

// c.f. https://en.wikipedia.org/wiki/Machine-readable_passport
// Passport booklets
MrzItem vTD3{"TD3",
             44,
             {
                 {Field::Type, 0, 0, 2, fixAlpha},
                 {Field::Country, 0, 2, 3, fixAlpha},
                 {Field::NameCombined, 0, 5, 39, fixAlpha},
                 {Field::Number, 1, 0, 9, fixNone},
                 {Field::CheckNumber, 1, 9, 1, fixNumeric},
                 {Field::Nationality, 1, 10, 3, fixAlpha},
                 {Field::DateOfBirth, 1, 13, 6, fixNumeric},
                 {Field::CheckDateOfBirth, 1, 19, 1, fixNumeric},
                 {Field::Sex, 1, 20, 1, fixAlpha},
                 {Field::ExpirationDate, 1, 21, 6, fixNumeric},
                 {Field::CheckExpirationDate, 1, 27, 1, fixNumeric},
                 {Field::PersonalNumber, 1, 28, 14, fixNone},
                 {Field::CheckPersonalNumber, 1, 42, 1, fixNumeric},
                 {Field::CheckCompLine2, 1, 43, 1, fixNumeric},
             }};
MrzItem vTD2{"TD2",
             36,
             {
                 {Field::Type, 0, 0, 2, fixAlpha},
                 {Field::Country, 0, 2, 3, fixAlpha},
                 {Field::NameCombined, 0, 5, 31, fixAlpha},
                 {Field::Number, 1, 0, 9, fixNone},
                 {Field::CheckNumber, 1, 9, 1, fixNumeric},
                 {Field::Nationality, 1, 10, 3, fixAlpha},
                 {Field::DateOfBirth, 1, 13, 6, fixNumeric},
                 {Field::CheckDateOfBirth, 1, 19, 1, fixNumeric},
                 {Field::Sex, 1, 20, 1, fixAlpha},
                 {Field::ExpirationDate, 1, 21, 6, fixNumeric},
                 {Field::CheckExpirationDate, 1, 27, 1, fixNumeric},
                 {Field::Optional1, 1, 28, 7, fixNone},
                 {Field::CheckCompLine2, 1, 35, 1, fixNumeric},
             }};
// Official travel documents
MrzItem vTD1{"TD1",
             30,
             {
                 {Field::Type, 0, 0, 2, fixAlpha},
                 {Field::Country, 0, 2, 3, fixAlpha},
                 {Field::Number, 0, 5, 9, fixNone},
                 {Field::CheckNumber, 0, 14, 1, fixNumeric},
                 {Field::Optional1, 0, 15, 15, fixNone},
                 {Field::DateOfBirth, 1, 0, 6, fixNumeric},
                 {Field::CheckDateOfBirth, 1, 6, 1, fixNumeric},
                 {Field::Sex, 1, 7, 1, fixAlpha},
                 {Field::ExpirationDate, 1, 8, 6, fixNumeric},
                 {Field::CheckExpirationDate, 1, 14, 1, fixNumeric},
                 {Field::Nationality, 1, 15, 3, fixAlpha},
                 {Field::Optional2, 1, 18, 11, fixNone},
                 {Field::CheckCompLine12, 1, 29, 1, fixNumeric},
                 {Field::NameCombined, 2, 0, 30, fixAlpha},
             }};
// Machine-readable visas
MrzItem vMRVA{"MRVA",
              44,
              {
                  {Field::Type, 0, 0, 1, fixAlpha},
                  {Field::Country, 0, 1, 2, fixAlpha},
                  {Field::NameCombined, 0, 5, 39, fixAlpha},
                  {Field::Number, 1, 0, 9, fixNone},
                  {Field::CheckNumber, 1, 9, 1, fixNumeric},
                  {Field::Nationality, 1, 10, 3, fixAlpha},
                  {Field::DateOfBirth, 1, 13, 6, fixNumeric},
                  {Field::CheckDateOfBirth, 1, 19, 1, fixNumeric},
                  {Field::Sex, 1, 20, 1, fixAlpha},
                  {Field::ExpirationDate, 1, 21, 6, fixNumeric},
                  {Field::CheckExpirationDate, 1, 27, 1, fixNumeric},
                  {Field::Optional1, 1, 28, 16, fixNone},
              }};
MrzItem vMRVB{"MRVB",
              36,
              {
                  {Field::Type, 0, 0, 1, fixAlpha},
                  {Field::Country, 0, 1, 2, fixAlpha},
                  {Field::NameCombined, 0, 5, 31, fixAlpha},
                  {Field::Number, 1, 0, 9, fixNone},
                  {Field::CheckNumber, 1, 9, 1, fixNumeric},
                  {Field::Nationality, 1, 10, 3, fixAlpha},
                  {Field::DateOfBirth, 1, 13, 6, fixNumeric},
                  {Field::CheckDateOfBirth, 1, 19, 1, fixNumeric},
                  {Field::Sex, 1, 20, 1, fixAlpha},
                  {Field::ExpirationDate, 1, 21, 6, fixNumeric},
                  {Field::CheckExpirationDate, 1, 27, 1, fixNumeric},
                  {Field::Optional1, 1, 28, 8, fixNone},
              }};
MrzItem vEmpty{"empty", {}};

vector<string> split(string const &datastr) {
  string_view data = datastr;
  vector<string> tmplines;
  while (data.size() > 0) {
    size_t idx = data.find('\n');
    if (idx > 28) {
      string_view sv = data.substr(0, idx);
      string line(sv.data(), sv.size());
      // guarantee consistent format
      replace(line.begin(), line.end(), ' ', '<');
      // guarantee enough characters (exceeding here)
      line.insert(line.end(), 50 - line.size(), '<');
      tmplines.push_back(line);
      printf("%s\n", string(data.substr(0, idx)).c_str());
    }
    if (idx != string_view::npos) {
      data.remove_prefix(idx + 1);
    }
  }
  return tmplines;
}
} // namespace

Mrz::Mrz(string const &datastr) : _values() {
  vector<string> origlines = split(datastr);
  for (size_t lo = 0; lo <= origlines.size() - 2; lo++) {
    // printf("loop: %zd/%zd\n", lo, origlines.size() - 2);
    // sliding window
    vector<string> lines = origlines;
    lines.erase(lines.begin(), lines.begin() + lo);
    // printf("------lines=%zd\n", lines.size());
    for (size_t offset = 0; offset < 2; offset++) {
      // printf("------\n");
      for (string &line : lines) {
        line = line.substr(offset);
        // printf("  %s\n", string(line).c_str());
      }
      MrzItem const &mrzitem = guessType(lines);
      if (mrzitem.fields.empty()) {
        // printf("Not identified\n");
        continue;
      }
      _values[MrzType] = mrzitem.name;
      _values[Raw1] = lines.at(0);
      _values[Raw2] = lines.at(1);
      if (lines.size() > 2) {
        _values[Raw3] = lines.at(2);
      }
      for (MrzField const &item : mrzitem.fields) {
        string_view line = lines[item.line];
        line = line.substr(item.start, item.length);
        _values[item.field] = item.cleaner.fix(line, item.length);
        // printf("  __value %d: %s\n", item.field,
        //       item.cleaner.fix(line, item.length).c_str());
      }
      postprocessNames();
      uint8_t valid = validate();
      if (valid > 2) {
        printf("Using:\n%s\n", toJSON().c_str());
        return;
      } else {
        printf("Dropping:\n%s\n", toJSON().c_str());
      }
      _values.clear();
    }
  }
}

string const &Mrz::val(Field field) const {
  static string empty;
  auto it = _values.find(field);
  return (it == _values.end()) ? empty : it->second;
}

std::string Mrz::toJSON() const {
  ostringstream ostr;
  toJSON(ostr);
  return ostr.str();
}

void Mrz::toJSON(ostream &os) const {

  Poco::JSON::Object ret;
  ret.set("mrz_type", val(Field::MrzType));
  ret.set("type", val(Field::Type));
  ret.set("country", val(Field::Country));
  ret.set("surname", val(Field::Surname));
  ret.set("names", val(Field::Names));
  ret.set("expiration_date", val(Field::ExpirationDate));
  ret.set("check_expiration_date", val(Field::CheckExpirationDate));
  ret.set("number", val(Field::Number));
  ret.set("check_number", val(Field::CheckNumber));
  Poco::JSON::Array raw;
  raw.set(0, val(Field::Raw1));
  raw.set(1, val(Field::Raw2));
  if (_values.find(Field::Raw3) != _values.end()) {
    raw.set(2, val(Field::Raw3));
  }
  ret.set("raw", raw);

  ret.stringify(os, 2);
}

uint8_t Mrz::validate() const {
  bool validNumber = isValid(CheckNumber, {Number});
  bool validDateOfBirth = isValid(CheckDateOfBirth, {DateOfBirth});
  bool validExpirationDate = isValid(CheckExpirationDate, {ExpirationDate});
  bool validSex = (_values.at(Sex).find_first_of("MF<") != string::npos);
  bool validAll =
      validNumber && validDateOfBirth && validExpirationDate && validSex;
  // TBD: bool CheckPersonalNumber etc
  if (_values.find(CheckCompLine12) != _values.end()) {
    bool valid =
        isValid(CheckCompLine12,
                {Number, CheckNumber, Optional1, DateOfBirth, CheckDateOfBirth,
                 ExpirationDate, CheckExpirationDate, Optional2});
    validAll &= valid;
  }
  return int(validNumber) + int(validDateOfBirth) + int(validExpirationDate) +
         int(validSex);
}

bool Mrz::isValid(Field check, initializer_list<Field> fields) const {
  string val;
  for (Field field : fields) {
    val += _values.at(field);
  }
  printf("  ..valid %d: %c == %s\n", check, MrzValid::calc(val),
         _values.at(check).c_str());
  return (MrzValid::calc(val) == _values.at(check)[0]);
}

void Mrz::postprocessNames() {
  string combined = _values[NameCombined];
  size_t idx = combined.find("<<");
  replace(combined.begin(), combined.end(), '<', ' ');
  if (idx != string::npos) {
    _values[Surname] = combined.substr(0, idx);
    size_t end = combined.find_last_not_of(' ', idx + 2);
    _values[Names] = combined.substr(idx + 2, end + 1);
  } else {
    _values[Surname] = combined;
  }
}

MrzItem const &Mrz::guessType(vector<string> const &lines) {
  if (lines.size() == 3) {
    return vTD1;
  }
  if (lines.size() == 2) {
    bool isV = (lines[0][0] == 'V');
    if (lines[0].size() < 40) {
      return isV ? vMRVB : vTD2;
    } else {
      return isV ? vMRVA : vTD3;
    }
  }
  return vEmpty;
}
