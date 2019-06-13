#include <Poco/Base64Decoder.h>
#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>
#include <Poco/Util/ServerApplication.h>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Mrz.hpp"
#include "Tesseract.hpp"

using namespace Poco::Net;
using namespace Poco::Util;
using namespace Poco::JSON;
using namespace Poco::Dynamic;
using namespace std;

namespace {
const unordered_map<string, string> existingfiles = {
#include "files.inc"
};
} // namespace

class MrzRequestHandler : public HTTPRequestHandler {
  Tesseract &_tess;
  Parser parser;

public:
  MrzRequestHandler(Tesseract &tess) : _tess(tess){};
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) {
    try {
      string data;
      Poco::StreamCopier::copyToString(req.stream(), data);
      // parse JSON
      Var result = parser.parse(data);
      auto object = result.extract<Object::Ptr>();
      string fileData = object->getValue<string>("media");
      istringstream instream(fileData);
      Poco::Base64Decoder decoder(instream);
      string decoded;
      Poco::StreamCopier::copyToString(decoder, decoded);
      // OCR
      string ret = _tess.analyze(decoded);
      // MRZ
      Mrz mrz(ret);
      if (mrz.valid()) {
        resp.setStatus(HTTPResponse::HTTP_OK);
        resp.setContentType("application/json");
        ostream &out = resp.send();
        mrz.toJSON(out);
        out.flush();
      } else {
        resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
      }
    } catch (...) {
      resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
    }
  }
};

class FileRequestHandler : public HTTPRequestHandler {
public:
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) {
    string base = ".";
    string uri = req.getURI();
    if (uri == "/") {
      resp.redirect("/static/index.html");
      return;
    } else if (uri == "/favicon.ico") {
      uri = "/static" + uri;
    }
    auto it = existingfiles.find(uri);
    if (it == existingfiles.end()) {
      resp.setStatus(HTTPResponse::HTTP_NOT_FOUND);
      return;
    }
    string fn = base + it->first;
    if (req.get("Accept-Encoding", "").find("gzip") != string::npos) {
      auto itgz = existingfiles.find(uri + ".gz");
      if (itgz != existingfiles.end()) {
        resp.set("Content-Encoding", "gzip");
        fn = base + itgz->first;
      }
    }
    resp.setStatus(HTTPResponse::HTTP_OK);
    resp.sendFile(fn, it->second);
  }
};

class RequestHandlerFactory : public HTTPRequestHandlerFactory {
  Tesseract _tess;

public:
  virtual HTTPRequestHandler *
  createRequestHandler(const HTTPServerRequest &req) {
    if (req.getURI() == "/mrz" && (req.getMethod() == HTTPRequest::HTTP_POST)) {
      return new MrzRequestHandler(_tess);
    } else if (req.getMethod() == HTTPRequest::HTTP_GET) {
      return new FileRequestHandler();
    }
    return nullptr;
  }
};

class MrzServerApp : public ServerApplication {
protected:
  int main(const vector<string> &) {
    int port = 8080;
    HTTPServerParams *pParams = new HTTPServerParams;
    pParams->setKeepAlive(false);
    pParams->setMaxThreads(4);
    HTTPServer s(new RequestHandlerFactory, ServerSocket(port), pParams);
    s.start();
    cout << endl << "Server started on port " << port << endl;
    waitForTerminationRequest(); // wait for CTRL-C or kill
    s.stop();
    return Application::EXIT_OK;
  }
};

int main(int argc, char **argv) {
  MrzServerApp app;
  return app.run(argc, argv);
}
