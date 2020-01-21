#include "Tesseract.hpp"
#include <iostream>
#include <leptonica/allheaders.h>
#include <mutex>
#include <string>
#include <tesseract/baseapi.h>

Tesseract::Tesseract() {
  api = new tesseract::TessBaseAPI();
  if (api->Init("tesseract/lang", "ocrb_int")) {
    fprintf(stderr, "Could not initialize tesseract.\n");
    exit(1);
  }
  api->SetPageSegMode(tesseract::PSM_AUTO);
}

Tesseract::~Tesseract() {
  // Destroy used object and release memory
  api->End();
}

std::string Tesseract::analyze(std::string const &data) {
  PIX *image = pixReadMem((const l_uint8 *)data.data(), data.size());
  std::lock_guard<std::mutex> guard(mtx);
  api->SetImage(image);
  if (api->GetSourceYResolution() == 0) {
    api->SetSourceResolution(92);
  }
  const char *outText = api->GetUTF8Text();
  std::string ret(outText);
  delete[] outText;
  pixDestroy(&image);
  return ret;
}
