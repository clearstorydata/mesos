#ifndef __DECODER_HPP__
#define __DECODER_HPP__

#include <http_parser.h>

#include <deque>
#include <string>
#include <vector>

#include <process/http.hpp>
#include <process/socket.hpp>

#include <stout/foreach.hpp>


// TODO(bmahler): Upgrade our http_parser to the latest version.
namespace process {

// TODO: Make DataDecoder abstract and make RequestDecoder a concrete subclass.
class DataDecoder
{
public:
  DataDecoder(const Socket& _s)
    : s(_s), failure(false), request(NULL)
  {
    settings.on_message_begin = &DataDecoder::on_message_begin;
    settings.on_header_field = &DataDecoder::on_header_field;
    settings.on_header_value = &DataDecoder::on_header_value;
    settings.on_path = &DataDecoder::on_path;
    settings.on_url = &DataDecoder::on_url;
    settings.on_fragment = &DataDecoder::on_fragment;
    settings.on_query_string = &DataDecoder::on_query_string;
    settings.on_body = &DataDecoder::on_body;
    settings.on_headers_complete = &DataDecoder::on_headers_complete;
    settings.on_message_complete = &DataDecoder::on_message_complete;

    http_parser_init(&parser, HTTP_REQUEST);

    parser.data = this;
  }

  std::deque<http::Request*> decode(const char* data, size_t length)
  {
    size_t parsed = http_parser_execute(&parser, &settings, data, length);

    if (parsed != length) {
      failure = true;
    }

    if (!requests.empty()) {
      std::deque<http::Request*> result = requests;
      requests.clear();
      return result;
    }

    return std::deque<http::Request*>();
  }

  bool failed() const
  {
    return failure;
  }

  Socket socket() const
  {
    return s;
  }

private:
  static int on_message_begin(http_parser* p)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;

    assert(!decoder->failure);

    decoder->header = HEADER_FIELD;
    decoder->field.clear();
    decoder->value.clear();
    decoder->query.clear();

    assert(decoder->request == NULL);
    decoder->request = new http::Request();
    decoder->request->headers.clear();
    decoder->request->method.clear();
    decoder->request->path.clear();
    decoder->request->url.clear();
    decoder->request->fragment.clear();
    decoder->request->query.clear();
    decoder->request->body.clear();

    return 0;
  }

  static int on_headers_complete(http_parser* p)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    decoder->request->method = http_method_str((http_method) decoder->parser.method);
    decoder->request->keepAlive = http_should_keep_alive(&decoder->parser);
    return 0;
  }

  static int on_message_complete(http_parser* p)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
//     std::cout << "http::Request:" << std::endl;
//     std::cout << "  method: " << decoder->request->method << std::endl;
//     std::cout << "  path: " << decoder->request->path << std::endl;
    // Parse the query key/values.
    decoder->request->query = http::query::parse(decoder->query);
    decoder->requests.push_back(decoder->request);
    decoder->request = NULL;
    return 0;
  }

  static int on_header_field(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);

    if (decoder->header != HEADER_FIELD) {
      decoder->request->headers[decoder->field] = decoder->value;
      decoder->field.clear();
      decoder->value.clear();
    }

    decoder->field.append(data, length);
    decoder->header = HEADER_FIELD;

    return 0;
  }

  static int on_header_value(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);
    decoder->value.append(data, length);
    decoder->header = HEADER_VALUE;
    return 0;
  }

  static int on_path(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);
    decoder->request->path.append(data, length);
    return 0;
  }

  static int on_url(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);
    decoder->request->url.append(data, length);
    return 0;
  }

  static int on_query_string(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);
    decoder->query.append(data, length);
    return 0;
  }

  static int on_fragment(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);
    decoder->request->fragment.append(data, length);
    return 0;
  }

  static int on_body(http_parser* p, const char* data, size_t length)
  {
    DataDecoder* decoder = (DataDecoder*) p->data;
    assert(decoder->request != NULL);
    decoder->request->body.append(data, length);
    return 0;
  }

  const Socket s; // The socket this decoder is associated with.

  bool failure;

  http_parser parser;
  http_parser_settings settings;

  enum {
    HEADER_FIELD,
    HEADER_VALUE
  } header;

  std::string field;
  std::string value;
  std::string query;

  http::Request* request;

  std::deque<http::Request*> requests;
};


class ResponseDecoder
{
public:
  ResponseDecoder()
    : failure(false), header(HEADER_FIELD), response(NULL)
  {
    settings.on_message_begin = &ResponseDecoder::on_message_begin;
    settings.on_header_field = &ResponseDecoder::on_header_field;
    settings.on_header_value = &ResponseDecoder::on_header_value;
    settings.on_path = &ResponseDecoder::on_path;
    settings.on_url = &ResponseDecoder::on_url;
    settings.on_fragment = &ResponseDecoder::on_fragment;
    settings.on_query_string = &ResponseDecoder::on_query_string;
    settings.on_body = &ResponseDecoder::on_body;
    settings.on_headers_complete = &ResponseDecoder::on_headers_complete;
    settings.on_message_complete = &ResponseDecoder::on_message_complete;

    http_parser_init(&parser, HTTP_RESPONSE);

    parser.data = this;
  }

  std::deque<http::Response*> decode(const char* data, size_t length)
  {
    size_t parsed = http_parser_execute(&parser, &settings, data, length);

    if (parsed != length) {
      failure = true;
    }

    if (!responses.empty()) {
      std::deque<http::Response*> result = responses;
      responses.clear();
      return result;
    }

    return std::deque<http::Response*>();
  }

  bool failed() const
  {
    return failure;
  }

private:
  static int on_message_begin(http_parser* p)
  {
    ResponseDecoder* decoder = (ResponseDecoder*) p->data;

    assert(!decoder->failure);

    decoder->header = HEADER_FIELD;
    decoder->field.clear();
    decoder->value.clear();

    assert(decoder->response == NULL);
    decoder->response = new http::Response();
    decoder->response->status.clear();
    decoder->response->headers.clear();
    decoder->response->type = http::Response::BODY;
    decoder->response->body.clear();
    decoder->response->path.clear();

    return 0;
  }

  static int on_headers_complete(http_parser* p)
  {
    return 0;
  }

  static int on_message_complete(http_parser* p)
  {
    ResponseDecoder* decoder = (ResponseDecoder*) p->data;
    hashmap<uint16_t, std::string>::const_iterator it =
        http::statuses.find(decoder->parser.status_code);
    assert(it != http::statuses.end());

    decoder->response->status = it->second;
    decoder->responses.push_back(decoder->response);
    decoder->response = NULL;
    return 0;
  }

  static int on_header_field(http_parser* p, const char* data, size_t length)
  {
    ResponseDecoder* decoder = (ResponseDecoder*) p->data;
    assert(decoder->response != NULL);

    if (decoder->header != HEADER_FIELD) {
      decoder->response->headers[decoder->field] = decoder->value;
      decoder->field.clear();
      decoder->value.clear();
    }

    decoder->field.append(data, length);
    decoder->header = HEADER_FIELD;

    return 0;
  }

  static int on_header_value(http_parser* p, const char* data, size_t length)
  {
    ResponseDecoder* decoder = (ResponseDecoder*) p->data;
    assert(decoder->response != NULL);
    decoder->value.append(data, length);
    decoder->header = HEADER_VALUE;
    return 0;
  }

  static int on_path(http_parser* p, const char* data, size_t length)
  {
    return 0;
  }

  static int on_url(http_parser* p, const char* data, size_t length)
  {
    return 0;
  }

  static int on_query_string(http_parser* p, const char* data, size_t length)
  {
    return 0;
  }

  static int on_fragment(http_parser* p, const char* data, size_t length)
  {
    return 0;
  }

  static int on_body(http_parser* p, const char* data, size_t length)
  {
    ResponseDecoder* decoder = (ResponseDecoder*) p->data;
    assert(decoder->response != NULL);
    decoder->response->body.append(data, length);
    return 0;
  }

  bool failure;

  http_parser parser;
  http_parser_settings settings;

  enum {
    HEADER_FIELD,
    HEADER_VALUE
  } header;

  std::string field;
  std::string value;

  http::Response* response;

  std::deque<http::Response*> responses;
};


}  // namespace process {

#endif // __DECODER_HPP__
