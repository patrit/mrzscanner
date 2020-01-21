#ifndef MRZ_TESSERACT_H
#define MRZ_TESSERACT_H

#include <mutex>
#include <string>
#include <tesseract/baseapi.h>

class Tesseract {
public:
  Tesseract();
  ~Tesseract();

  std::string analyze(std::string const &data);

private:
  std::mutex mtx;
  tesseract::TessBaseAPI *api = nullptr;
};

#endif