#ifndef MRZ_VALID_H
#define MRZ_VALID_H

#include <string>

// c.f. https://www.icao.int/publications/Documents/9303_p3_cons_en.pdf
// ch 4.9 Check Digits in the MRZ

class MrzValid {

public:
  MrzValid() = delete;
  ~MrzValid() = delete;
  static char calc(std::string const &s);

private:
  static uint8_t val(char c);
};

#endif