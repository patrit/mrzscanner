#include "Mrz.hpp"

#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <algorithm>
#include <initializer_list>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Countries.hpp"
#include "MrzValid.hpp"

using namespace std;

namespace {

enum Split { KeepSpace, DropSpace, FillSpace };

MrzCleaner fixNone(MrzCleaner::none);
MrzCleaner fixAlpha(MrzCleaner::alpha);
MrzCleaner fixNumeric(MrzCleaner::numeric);

// c.f. https://en.wikipedia.org/wiki/Machine-readable_passport
// Passport booklets
static MrzItem vTD3{"TD3",
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
                        {Field::CheckCompLine2_td3, 1, 43, 1, fixNumeric},
                    }};
static MrzItem vTD2{"TD2",
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
                        {Field::CheckCompLine2_td2, 1, 35, 1, fixNumeric},
                    }};
// Official travel documents
static MrzItem vTD1{"TD1",
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
static MrzItem vMRVA{"MRVA",
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
                         {Field::Optional1, 1, 28, 16, fixNone},
                     }};
static MrzItem vMRVB{"MRVB",
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
                         {Field::Optional1, 1, 28, 8, fixNone},
                     }};
static MrzItem vIDFRA{"IDFRA",
                      36,
                      {
                          {Field::Type, 0, 0, 2, fixAlpha},
                          {Field::Country, 0, 2, 3, fixAlpha},
                          {Field::Surname, 0, 5, 25, fixAlpha},
                          {Field::Optional1, 0, 30, 6, fixNone},
                          {Field::CreationDate, 1, 0, 4, fixNumeric},
                          {Field::Optional2, 1, 4, 3, fixNone},
                          {Field::Number, 1, 7, 5, fixNumeric},
                          {Field::CheckCompLine2_fra, 1, 12, 1, fixNumeric},
                          {Field::Names, 1, 13, 14, fixAlpha},
                          {Field::DateOfBirth, 1, 27, 6, fixNumeric},
                          {Field::CheckDateOfBirth, 1, 33, 1, fixNumeric},
                          {Field::Sex, 1, 34, 1, fixNone},
                          {Field::CheckCompLine12_fra, 1, 35, 1, fixNumeric},
                      }};
static MrzItem vEmpty{"empty", {}};

unordered_map<Field, vector<Field>> _checks = {
    {Field::CheckNumber, {Field::Number}},
    {Field::CheckPersonalNumber, {Field::PersonalNumber}},
    {Field::CheckDateOfBirth, {Field::DateOfBirth}},
    {Field::CheckExpirationDate, {Field::ExpirationDate}},
    {Field::CheckCompLine2_td2,
     {Field::Number, Field::CheckNumber, Field::DateOfBirth,
      Field::CheckDateOfBirth, Field::ExpirationDate,
      Field::CheckExpirationDate, Field::Optional1}},
    {Field::CheckCompLine2_td3,
     {Field::Number, Field::CheckNumber, Field::DateOfBirth,
      Field::CheckDateOfBirth, Field::ExpirationDate,
      Field::CheckExpirationDate, Field::PersonalNumber,
      Field::CheckPersonalNumber}},
    {Field::CheckCompLine12,
     {Field::Number, Field::CheckNumber, Field::Optional1, Field::DateOfBirth,
      Field::CheckDateOfBirth, Field::ExpirationDate,
      Field::CheckExpirationDate, Field::Optional2}},
    {Field::CheckCompLine2_fra,
     {Field::CreationDate, Field::Optional2, Field::Number}},
    {Field::CheckCompLine12_fra,
     {Field::Type, Field::Country, Field::Surname, Field::Optional1,
      Field::CreationDate, Field::Optional2, Field::Number,
      Field::CheckCompLine2_fra, Field::Names, Field::DateOfBirth,
      Field::CheckDateOfBirth, Field::Sex}}};

vector<string> split(string const &datastr, Split splittype) {
  string_view data = datastr;
  vector<string> tmplines;
  while (data.size() > 0) {
    size_t idx = data.find('\n');
    if ((idx > 28 && idx < 50) ||
        ((idx == string_view::npos) && data.size() > 28 && data.size() < 50)) {
      string_view sv = data.substr(0, idx);
      string line(sv.data(), sv.size());
      // guarantee consistent format
      switch (splittype) {
      case Split::DropSpace:
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        break;
      case Split::FillSpace:
        replace(line.begin(), line.end(), ' ', '<');
        break;
      case Split::KeepSpace:
        break;
      }
      tmplines.push_back(line);
      // printf("%s\n", string(data.substr(0, idx)).c_str());
    }
    if (idx == string_view::npos) {
      break;
    }
    data.remove_prefix(idx + 1);
  }
  return tmplines;
}
} // namespace

Mrz::Mrz(string const &datastr, bool debug)
    : _datastring(datastr), _debug(debug), _debugstr(), _values() {
  if (_debug) {
    _debugstr.reserve(32768);
    _debugstr += "Parsed string:\n" + datastr;
  }
  for (Split splittype : {Split::DropSpace, Split::FillSpace}) {
    vector<string> origlines = split(datastr, splittype);
    for (size_t numlines : {3, 2}) {
      for (size_t lo = 0; lo + numlines <= origlines.size(); lo++) {
        if (_debug) {
          _debugstr += "\nIteration: #lines=" + to_string(numlines);
          _debugstr += ", loop=" + to_string(lo) + "/";
          _debugstr += to_string(origlines.size() - numlines) + "\n";
        }
        // sliding window
        vector<string> lines;
        copy(origlines.begin() + lo, origlines.begin() + lo + numlines,
             std::back_inserter(lines));
        for (size_t offset = 0; offset < 2; offset++) {
          if (_debug) {
            _debugstr += "Lines:\n";
          }
          for (string &line : lines) {
            line = line.substr(offset);
            if (_debug) {
              _debugstr += line + "\n";
            }
          }
          MrzItem const &mrzitem = guessType(lines);
          if (mrzitem.fields.empty()) {
            if (_debug) {
              _debugstr += "Unable to identify mrz type\n";
            }
            continue;
          }
          for (string &line : lines) {
            if (line.size() < mrzitem.length) {
              line.insert(line.end(), mrzitem.length - line.size(), '<');
            }
          }
          _values[Field::MrzType] = mrzitem.name;
          _values[Field::Raw1] = lines.at(0);
          _values[Field::Raw2] = lines.at(1);
          if (lines.size() > 2) {
            _values[Field::Raw3] = lines.at(2);
          }
          for (MrzField const &item : mrzitem.fields) {
            string_view line = lines[item.line];
            line = line.substr(item.start, item.length);
            // generic fixup
            _values[item.field] = item.cleaner.fix(line);
          }
          auto [valid, num] = validate();
          if (_debug) {
            _debugstr +=
                "validate=" + to_string(valid) + "/" + to_string(num) + "\n";
          }
          if (valid == num) {
            postprocessNames();
            if (_debug) {
              _debugstr += "Using:\n";
              _debugstr += toJSON() + "\n";
            }
            return;
          } else {
            if (_debug) {
              _debugstr += "Dropping:\n";
              _debugstr += toJSON() + "\n";
            }
          }
          _values.clear();
        }
      }
    }
  }
}

std::string Mrz::toJSON() const {
  ostringstream ostr;
  toJSON(ostr);
  return ostr.str();
}

void Mrz::toJSON(ostream &os) const {
  Poco::JSON::Object ret;
  if (_values.empty()) {
    Poco::JSON::Array raw;
    vector<string> lines = split(_datastring, Split::KeepSpace);
    size_t idx = 0;
    for (string const &line : lines) {
      raw.set(idx, line);
      idx++;
    }
    ret.set("raw", raw);
    ret.stringify(os, 2);
  } else {
    for (auto const &[field, fieldname] : MrzFields::getMap()) {
      auto it = _values.find(field);
      if (it != _values.end() && !fieldname.empty()) {
        MrzValue const &val = it->second;
        if (val.isCheck()) {
          Poco::JSON::Object check;
          check.set("check_expected", val.getCheckExpected());
          check.set("check_calculated", val.getCheckCalculated());
          check.set("concatenated_string", val.getConcatenatedString());
          check.set("is_valid", val.isValid());
          ret.set(fieldname, check);
        } else {
          ret.set(fieldname, val.getValue());
        }
      }
    }
    Poco::JSON::Array raw;
    raw.set(0, _values.at(Field::Raw1).getValue());
    raw.set(1, _values.at(Field::Raw2).getValue());
    if (_values.find(Field::Raw3) != _values.end()) {
      raw.set(2, _values.at(Field::Raw3).getValue());
    }
    ret.set("raw", raw);
    ret.stringify(os, 2);
  }
}

std::tuple<uint8_t, uint8_t> Mrz::validate() {
  uint8_t num = 1;
  uint8_t numValid =
      int(_values.at(Sex).getValue().find_first_of("MF<") != string::npos);
  num++;
  numValid += int(Countries::has_country(_values.at(Country).getValue()));
  for (auto const &[check, checkfields] : _checks) {
    auto it = _values.find(check);
    if (it != _values.end()) {
      MrzValue &mrzval = it->second;
      string val = concatCheckString(check, checkfields);
      char checkCalculated = MrzValid::calc(val);
      mrzval.updateCheck(checkCalculated, val);
      char checkExpected = mrzval.getCheckExpected();
      num++;
      numValid += int((mrzval.isValid()) ? 1 : 0);
    }
  }
  return std::make_tuple(numValid, num);
}

string Mrz::concatCheckString(Field check, vector<Field> const &fields) const {
  string val;
  for (Field field : fields) {
    if (_values.find(field) == _values.end()) {
      printf("error: field %d not found in check %d\n", field, check);
      return string();
    }
    val += _values.at(field).getValue();
  }
  return val;
}

void Mrz::postprocessNames() {
  if (_values.find(Field::NameCombined) != _values.end()) {
    string combined = _values[Field::NameCombined].getValue();
    size_t idx = combined.find("<<");
    replace(combined.begin(), combined.end(), '<', ' ');
    if (idx != string::npos) {
      _values[Field::Surname] = combined.substr(0, idx);
      combined = combined.substr(idx + 2);
      size_t end = combined.find_last_not_of(' ');
      size_t end2 = combined.find("  ");
      _values[Field::Names] = combined.substr(0, min(end + 1, end2));
    } else {
      _values[Field::Surname] = combined;
    }
  } else {
    string names = _values[Field::Names].getValue();
    replace(names.begin(), names.end(), '<', ' ');
    size_t end = names.find_last_not_of(' ');
    _values[Field::Names] = names.substr(0, end + 1);
    string surname = _values[Surname].getValue();
    replace(surname.begin(), surname.end(), '<', ' ');
    end = surname.find_last_not_of(' ');
    _values[Field::Surname] = surname.substr(0, end + 1);
  }
}

MrzItem const &Mrz::guessType(vector<string> const &lines) {
  if (lines.size() == 3) {
    return vTD1;
  }
  if (lines.size() == 2) {
    bool isV = (lines[0][0] == 'V');
    if (lines[0].size() >= 40 || lines[1].size() >= 40) {
      return isV ? vMRVA : vTD3;
    } else if (lines[0].size() > 26 || lines[1].size() > 26) {
      if (lines[0].substr(0, 5) == "IDFRA") {
        return vIDFRA;
      }
      return isV ? vMRVB : vTD2;
    }
  }
  return vEmpty;
}
