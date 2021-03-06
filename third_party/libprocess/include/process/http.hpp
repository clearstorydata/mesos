#ifndef __PROCESS_HTTP_HPP__
#define __PROCESS_HTTP_HPP__

#include <string>

#include <process/future.hpp>
#include <process/pid.hpp>

#include <stout/hashmap.hpp>
#include <stout/json.hpp>
#include <stout/option.hpp>
#include <stout/stringify.hpp>
#include <stout/strings.hpp>
#include <stout/try.hpp>

namespace process {
namespace http {

struct Request
{
  // TODO(benh): Add major/minor version.
  hashmap<std::string, std::string> headers;
  std::string method;
  std::string path;
  std::string url;
  std::string fragment;
  hashmap<std::string, std::string> query;
  std::string body;
  bool keepAlive;
};


struct Response
{
  Response(const std::string& _body = "")
    : type(BODY),
      body(_body)
  {
    if (!body.empty()) {
      headers["Content-Length"] = stringify(body.size());
    }
  }

  // TODO(benh): Add major/minor version.
  std::string status;
  hashmap<std::string, std::string> headers;

  // TODO(benh): Make body a stream (channel) instead, and allow a
  // response to be returned without forcing the stream to be
  // finished.

  // Either provide a 'body' or an absolute 'path' to a file. If a
  // path is specified then we will attempt to perform a 'sendfile'
  // operation on the file. In either case you are expected to
  // properly specify the 'Content-Type' header, but the
  // 'Content-Length' header will be filled in for you if you specify
  // a path. Distinguish between the two using 'type' below.
  enum {
    BODY,
    PATH
  } type;

  std::string body;
  std::string path;
};


struct OK : Response
{
  OK(const std::string& body = "") : Response(body)
  {
    status = "200 OK";
  }

  OK(const JSON::Value& value, const Option<std::string>& jsonp) : Response()
  {
    status = "200 OK";

    std::ostringstream out;

    if (jsonp.isSome()) {
      out << jsonp.get() << "(";
    }

    JSON::render(out, value);

    if (jsonp.isSome()) {
      out << ");";
      headers["Content-Type"] = "text/javascript";
    } else {
      headers["Content-Type"] = "application/json";
    }

    headers["Content-Length"] = stringify(out.str().size());
    body = out.str().data();
  }
};


struct BadRequest : Response
{
  BadRequest(const std::string& body = "") : Response(body)
  {
    status = "400 Bad Request";
  }
};


struct NotFound : Response
{
  NotFound(const std::string& body = "") : Response(body)
  {
    status = "404 Not Found";
  }
};


struct InternalServerError : Response
{
  InternalServerError(const std::string& body = "") : Response(body)
  {
    status = "500 Internal Server Error";
  }
};


struct ServiceUnavailable : Response
{
  ServiceUnavailable(const std::string& body = "") : Response(body)
  {
    status = "503 Service Unavailable";
  }
};


struct TemporaryRedirect : Response
{
  TemporaryRedirect(const std::string& url) : Response("")
  {
    status = "307 Temporary Redirect";
    headers["Location"] = url;
  }
};


namespace query {

// Parses an HTTP query string into a map. For example:
//
//   parse("foo=1;bar=2;baz;foo=3")
//
// Would return a map with the following:
//   bar: "2"
//   baz: ""
//   foo: "3"
//
// We use the last value for a key for simplicity, since the RFC does not
// specify how to handle duplicate keys:
// http://en.wikipedia.org/wiki/Query_string
// TODO(bmahler): If needed, investigate populating the query map inline
// for better performance.
inline hashmap<std::string, std::string> parse(const std::string& query)
{
  hashmap<std::string, std::string> result;

  const std::vector<std::string>& tokens = strings::tokenize(query, ";&");
  foreach (const std::string& token, tokens) {
    const std::vector<std::string>& pairs = strings::split(token, "=");
    if (pairs.size() == 2) {
      result[pairs[0]] = pairs[1];
    } else if (pairs.size() == 1) {
      result[pairs[0]] = "";
    }
  }

  return result;
}

}  // namespace query {

// Sends a blocking HTTP GET request to the process with the given upid.
// Returns the HTTP response from the process, read asynchronously.
// The query is the string version of the path and arguments:
// eg. browse.json?path=sandbox
//
// TODO(bmahler): Have the request sent asynchronously as well.
// TODO(bmahler): For efficiency, this should properly use the ResponseDecoder
// on the read stream, rather than parsing the full string response at the end.
Future<Response> get(const PID<>& pid,
                     const std::string& query = "",
                     const std::string& body = "");


// Status code reason strings, from the HTTP1.1 RFC:
// http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html
extern hashmap<uint16_t, std::string> statuses;


inline void initialize()
{
  statuses[100] = "100 Continue";
  statuses[101] = "101 Switching Protocols";
  statuses[200] = "200 OK";
  statuses[201] = "201 Created";
  statuses[202] = "202 Accepted";
  statuses[203] = "203 Non-Authoritative Information";
  statuses[204] = "204 No Content";
  statuses[205] = "205 Reset Content";
  statuses[206] = "206 Partial Content";
  statuses[300] = "300 Multiple Choices";
  statuses[301] = "301 Moved Permanently";
  statuses[302] = "302 Found";
  statuses[303] = "303 See Other";
  statuses[304] = "304 Not Modified";
  statuses[305] = "305 Use Proxy";
  statuses[307] = "307 Temporary Redirect";
  statuses[400] = "400 Bad Request";
  statuses[401] = "401 Unauthorized";
  statuses[402] = "402 Payment Required";
  statuses[403] = "403 Forbidden";
  statuses[404] = "404 Not Found";
  statuses[405] = "405 Method Not Allowed";
  statuses[406] = "406 Not Acceptable";
  statuses[407] = "407 Proxy Authentication Required";
  statuses[408] = "408 Request Time-out";
  statuses[409] = "409 Conflict";
  statuses[410] = "410 Gone";
  statuses[411] = "411 Length Required";
  statuses[412] = "412 Precondition Failed";
  statuses[413] = "413 Request Entity Too Large";
  statuses[414] = "414 Request-URI Too Large";
  statuses[415] = "415 Unsupported Media Type";
  statuses[416] = "416 Requested range not satisfiable";
  statuses[417] = "417 Expectation Failed";
  statuses[500] = "500 Internal Server Error";
  statuses[501] = "501 Not Implemented";
  statuses[502] = "502 Bad Gateway";
  statuses[503] = "503 Service Unavailable";
  statuses[504] = "504 Gateway Time-out";
  statuses[505] = "505 HTTP Version not supported";
}


} // namespace http {
} // namespace process {

#endif // __PROCESS_HTTP_HPP__
