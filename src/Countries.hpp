#ifndef MRZ_COUNTRIES_H
#define MRZ_COUNTRIES_H

#include <iostream>
#include <string>

class Countries {
public:
  static bool has_country(std::string country);
  static void toJson(std::ostream &out);
};

#endif