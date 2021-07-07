#include "Tesseract.hpp"
#include <iostream>
#include <leptonica/allheaders.h>
#include <mutex>
#include <string>
#include <tesseract/baseapi.h>

Tesseract::Tesseract() {
  const char *lang = getenv("TESSERACT_LANG");
  if (lang == nullptr) {
    lang = "ocrb_int";
  }
  api = new tesseract::TessBaseAPI();
  if (api->Init("tesseract/lang", lang)) {
    fprintf(stderr, "Could not initialize tesseract.\n");
    exit(1);
  }
  api->SetPageSegMode(tesseract::PSM_AUTO);
  // api->SetVariable("tessedit_char_whitelist",
  //                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<");
}

Tesseract::~Tesseract() {
  // Destroy used object and release memory
  api->End();
  delete api;
}

std::string Tesseract::analyze(std::string const &data) {
  PIX *image = pixReadMem((const l_uint8 *)data.data(), data.size());
  std::lock_guard<std::mutex> guard(mtx);
  api->SetImage(image);
  if (api->GetSourceYResolution() == 0) {
    api->SetSourceResolution(300);
  }
  const char *outText = api->GetUTF8Text();
  std::string ret(outText);
  delete[] outText;
  pixDestroy(&image);
  return ret;
}
