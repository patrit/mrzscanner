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
#include <Poco/URI.h>
#include <Poco/Util/ServerApplication.h>
#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

#include "Countries.hpp"
#include "Mrz.hpp"
#include "Tesseract.hpp"

using namespace Poco::Net;
using namespace Poco::Util;
using namespace Poco::JSON;
using namespace Poco::Dynamic;
using namespace std;
namespace fs = std::experimental::filesystem;

class MrzRequestHandler : public HTTPRequestHandler {
  Tesseract &_tess;
  Parser parser;

public:
  MrzRequestHandler(Tesseract &tess) : _tess(tess){};
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) {
    Poco::URI uri(req.getURI());
    auto query = uri.getQueryParameters();
    bool debug = false;
    for (auto const &[key, val] : query) {
      if (key == "debugonly" && val == "true") {
        debug = true;
      }
    }
    string data;
    Poco::StreamCopier::copyToString(req.stream(), data);
    // parse JSON
    Var result = parser.parse(data);
    auto object = result.extract<Object::Ptr>();
    string file_data = object->getValue<string>("file_data");
    istringstream instream(file_data);
    Poco::Base64Decoder decoder(instream);
    string decoded;
    Poco::StreamCopier::copyToString(decoder, decoded);
    // OCR
    string ret = _tess.analyze(decoded);
    // MRZ
    Mrz mrz(ret, debug);

    resp.setStatus(mrz.detected() ? HTTPResponse::HTTP_OK
                                  : HTTPResponse::HTTP_BAD_REQUEST);
    ostream &out = resp.send();
    if (debug) {
      resp.setContentType("text/plain");
      out << mrz.getDebugString();
    } else {
      resp.setContentType("application/json");
      mrz.toJSON(out);
    }
    out.flush();
  }
};

class CountryRequestHandler : public HTTPRequestHandler {
public:
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) {
    resp.setStatus(HTTPResponse::HTTP_OK);
    resp.setContentType("application/json");
    Countries::toJson(resp.send());
    resp.send().flush();
  }
};

namespace {
unordered_map<string, string> mimetypes = {
    {".png", "image/png"},
    {".css", "text/css"},
    {".html", "text/html"},
    {".js", "application/javascript"},
};
string octet_stream("application/octet-stream");
} // namespace

class FileRequestHandler : public HTTPRequestHandler {
  string _path;

public:
  FileRequestHandler(string path) : _path(std::move(path)){};
  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) {
    string base = ".";
    string fname = base + _path;
    auto path = fs::path(fname);
    fs::file_status status = fs::status(path);
    if (fs::is_directory(status)) {
      fname += "index.html";
    }
    resp.setChunkedTransferEncoding(true);
    bool exists = fs::exists(fname);
    if (!exists) {
      resp.setStatus(HTTPResponse::HTTP_NOT_FOUND);
      return;
    }
    path = fs::path(fname);
    auto it = mimetypes.find(path.extension());
    if (it != mimetypes.end()) {
      resp.setContentType(it->second);
    } else {
      resp.setContentType(octet_stream);
    }
    // check if compressed version of the file exists
    string fnamegz = base + req.getURI() + ".gz";
    if (req.get("Accept-Encoding", "").find("gzip") != string::npos) {
      if (fs::exists(fnamegz)) {
        resp.set("Content-Encoding", "gzip");
        fname = fnamegz;
      }
    }
    resp.setStatus(HTTPResponse::HTTP_OK);
    Poco::FileInputStream fis(fname);
    ostream &out = resp.send();
    out << fis.rdbuf();
    out.flush();
  }
};

class RequestHandlerFactory : public HTTPRequestHandlerFactory {
  Tesseract _tess;

public:
  virtual HTTPRequestHandler *
  createRequestHandler(const HTTPServerRequest &req) {
    Poco::URI uri(req.getURI());
    if (uri.getPath() == "/mrz/api/v1/analyze_image" &&
        (req.getMethod() == HTTPRequest::HTTP_POST)) {
      return new MrzRequestHandler(_tess);
    } else if (uri.getPath() == "/mrz/api/v1/countries" &&
               (req.getMethod() == HTTPRequest::HTTP_GET)) {
      return new CountryRequestHandler();
    } else if (req.getMethod() == HTTPRequest::HTTP_GET) {
      return new FileRequestHandler(uri.getPath());
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
