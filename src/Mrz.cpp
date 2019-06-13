#include "Mrz.hpp"

#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Logger.h>
#include <algorithm>
#include <initializer_list>
#include <map>
#include <string_view>
#include <vector>

#include "MrzValid.hpp"

using namespace std;
using Poco::Logger;

namespace {

Logger &logger = Logger::get("MRZ");

enum Space { Erase, Fill };

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
                       {'R', '2'},
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

vector<string_view> split(string const &datastr) {
  string_view data = datastr;
  vector<string_view> tmplines;
  size_t begin = 0;
  size_t end = 0;
  while ((end = data.find('\n', begin)) != string_view::npos) {
    size_t len = end - begin;
    if (len > 28) {
      string_view line = data.substr(begin, len);
      tmplines.push_back(line);
      cout << line << end;
    }
    begin = end + 1;
  }
  return tmplines;
}

void fill(vector<string> &lines, Space spaceBhv) {
  for (string &line : lines) {
    // guarantee consistent format
    switch (spaceBhv) {
    case Space::Fill:
      replace(line.begin(), line.end(), ' ', '<');
      break;
    case Space::Erase:
      line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
      break;
    }
    // guarantee enough characters (exceeding here)
    line.insert(line.end(), 46 - line.size(), '<');
  }
}
} // namespace

Mrz::Mrz(string const &datastr) : _values(), _valid(false) {
  // do some brute force on OCR lines
  // remove spaces or replace them with a '<'
  for (Space spaceBhv : {Space::Erase, Space::Fill}) {
    vector<string_view> origlines = split(datastr);
    // either 2 or 3 lines
    for (size_t numlines : {3, 2}) {
      if (origlines.size() < numlines) {
        continue;
      }
      // sliding window: iterate over all lines
      for (size_t lo = 0; lo <= origlines.size() - numlines; lo++) {
        logger.debug("sliding window: 0..[%zd..%zd]..%zd", lo, lo + numlines,
                     origlines.size());
        vector<string> lines;
        std::transform(origlines.begin() + lo,
                       origlines.begin() + lo + numlines, back_inserter(lines),
                       [](string_view const &sv) { return string(sv); });
        // try to skip the first character - helps with spurious character
        // detection
        for (size_t offset = 0; offset < round(exp2(numlines)); offset++) {
          size_t off = offset;
          for (string &line : lines) {
            line = line.substr(off % 2);
            off = off / 2;
          }
          MrzItem const &mrzitem = guessType(lines);
          if (mrzitem.fields.empty()) {
            logger.debug("Not identified");
            continue;
          }
          // fill empty spaces
          fill(lines, spaceBhv);
          // fill fields
          _values[MrzType] = mrzitem.name;
          for (size_t idx = 0; idx < numlines; idx++) {
            Field field = Field(Raw1 + idx);
            _values[field] = lines.at(idx).substr(0, mrzitem.length);
          }
          for (MrzField const &item : mrzitem.fields) {
            string_view line = lines.at(item.line);
            line = line.substr(item.start, item.length);
            _values[item.field] = item.cleaner.fix(line, item.length);
          }
          postprocessNames();
          _valid = validate();
          if (_valid) {
            logger.information("Using MRZ:\n%s", rawString());
            return;
          } else {
            logger.debug("Invalid MRZ:\n%s", rawString());
            _values.clear();
          }
        }
      }
    }
  }
}

string const &Mrz::val(Field field) const {
  static string empty;
  auto it = _values.find(field);
  return (it == _values.end()) ? empty : it->second;
}

void Mrz::set(Poco::JSON::Object &sum, string const &key, Field field) const {
  auto it = _values.find(field);
  if (it != _values.end()) {
    string const &val = it->second;
    if (val.find_first_not_of('<') != string::npos) {
      sum.set(key, val);
    }
  }
}

bool Mrz::exists(Field field) const {
  return _values.find(field) != _values.end();
}

string Mrz::rawString() const {
  string ret = val(Field::Raw1) + '\n' + val(Field::Raw2);
  if (exists(Field::Raw3)) {
    ret += '\n' + val(Field::Raw3);
  }
  return ret;
}

string Mrz::toJSON() const {
  ostringstream ostr;
  toJSON(ostr);
  return ostr.str();
}

void Mrz::toJSON(ostream &os) const {
  Poco::JSON::Object ret;
  set(ret, "mrz_type", Field::MrzType);
  set(ret, "type", Field::Type);
  set(ret, "country", Field::Country);
  set(ret, "surname", Field::Surname);
  set(ret, "names", Field::Names);
  set(ret, "expiration_date", Field::ExpirationDate);
  set(ret, "check_expiration_date", Field::CheckExpirationDate);
  set(ret, "number", Field::Number);
  set(ret, "check_number", Field::CheckNumber);
  set(ret, "date_of_birth", Field::DateOfBirth);
  set(ret, "check_date_of_birth", Field::CheckDateOfBirth);
  set(ret, "nationality", Field::Nationality);
  set(ret, "sex", Field::Sex);
  set(ret, "personal_number", Field::PersonalNumber);
  set(ret, "check_personal_number", Field::CheckPersonalNumber);
  set(ret, "optional1", Field::Optional1);
  set(ret, "optional2", Field::Optional2);
  set(ret, "check_combined_2", Field::CheckCompLine2);
  set(ret, "check_combined_12", Field::CheckCompLine12);
  Poco::JSON::Array raw;
  raw.set(0, val(Field::Raw1));
  raw.set(1, val(Field::Raw2));
  if (exists(Field::Raw3)) {
    raw.set(2, val(Field::Raw3));
  }
  ret.set("raw", raw);

  ret.stringify(os, 2);
}

bool Mrz::validate() const {
  bool validNumber = isValid(CheckNumber, {Number});
  bool validDateOfBirth = isValid(CheckDateOfBirth, {DateOfBirth});
  bool validExpirationDate = isValid(CheckExpirationDate, {ExpirationDate});
  bool validSex = (_values.at(Sex).find_first_of("MF<") != string::npos);
  bool validAll =
      validNumber && validDateOfBirth && validExpirationDate && validSex;

  if (exists(CheckPersonalNumber)) {
    bool valid = isValid(CheckPersonalNumber, {PersonalNumber});
    validAll &= valid;
  }

  if (exists(CheckCompLine2)) {
    bool valid = isValid(CheckCompLine2,
                         {Number, CheckNumber, DateOfBirth, CheckDateOfBirth,
                          ExpirationDate, CheckExpirationDate, PersonalNumber,
                          CheckPersonalNumber});
    validAll &= valid;
  }

  if (exists(CheckCompLine12)) {
    bool valid =
        isValid(CheckCompLine12,
                {Number, CheckNumber, Optional1, DateOfBirth, CheckDateOfBirth,
                 ExpirationDate, CheckExpirationDate, Optional2});
    validAll &= valid;
  }
  return validAll;
}

bool Mrz::isValid(Field check, initializer_list<Field> fields) const {
  string val;
  auto it = _values.find(check);
  if (it == _values.end()) {
    //printf("  ..valid %d: invald\n", check);
    return false;
  }
  string const &checkstr = it->second;
  if (checkstr.empty()) {
    //printf("  ..valid %d: invald empty string\n", check);
    return false;
  }
  if (checkstr.at(0) != '<' && (checkstr.at(0) < '0' || checkstr.at(0) > '9')) {
    //printf("  ..valid %d: invald %s\n", check, checkstr.c_str());
    return false;
  }
  for (Field field : fields) {
    auto it = _values.find(field);
    if (it == _values.end()) {
      return false;
    }
    val += it->second;
  }
  //printf("  ..valid %d: %c == %s, val=%s\n", check, MrzValid::calc(val),
  //       checkstr.c_str(), val.c_str());
  if (checkstr.at(0) == '<') {
    return val.find_first_not_of('<') == string::npos;
  }
  return (MrzValid::calc(val) == checkstr[0]);
}

void Mrz::postprocessNames() {
  auto it = _values.find(NameCombined);
  if (it == _values.end()) {
    return;
  }
  string combined = it->second;
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
    bool isV = (lines.at(0).at(0) == 'V');
    if (lines[0].size() < 40) {
      return isV ? vMRVB : vTD2;
    } else {
      return isV ? vMRVA : vTD3;
    }
  }
  return vEmpty;
}
