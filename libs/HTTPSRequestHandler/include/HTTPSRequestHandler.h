#pragma once
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

using NameValueCollection = std::unordered_map<std::string, std::string>;

class Request {
  public:
    enum class Method { GET, POST };
    enum class OnRedirect { FOLLOW, DONT_FOLLOW };
    Request(Method method, std::string location, OnRedirect follow_redirects = OnRedirect::FOLLOW)
        : method_(method), location_(std::move(location)), follow_redirects_(follow_redirects) {}
    Request(std::string location) : Request(Method::GET, std::move(location)) {}
    virtual ~Request() = default;

    Method getMethod() const { return method_; }

    const std::string& getLocation() const { return location_; }

    OnRedirect getFollowRedirects() const { return follow_redirects_; }

  private:
    Method method_;
    std::string location_;
    OnRedirect follow_redirects_;
};

class Response {
  public:
    enum class Status : int {
        HTTP_OK = 200,
        HTTP_MULTIPLE_CHOICES = 300,
        HTTP_MOVED_PERMANENTLY = 301,
        HTTP_FOUND = 302,
    };

    template <typename Iterator>
    Response(Status status, std::string reason, Iterator start, const Iterator stop)
        : status_(status), reason_(reason), header_(start, stop) {}

    Status getStatus() const { return status_; }
    const std::string& getReason() const { return reason_; }
    bool has(const std::string& key) const { return header_.find(key) != header_.end(); }
    const std::string& operator[](const std::string& key) const { return header_.at(key); }

  private:
    Status status_;
    std::string reason_;
    std::unordered_map<std::string, std::string> header_;
};

inline bool operator<(Response::Status lhs, Response::Status rhs) {
    return static_cast<int>(lhs) < static_cast<int>(rhs);
}

inline bool operator>=(Response::Status lhs, Response::Status rhs) { return !(lhs < rhs); }

inline bool operator>(Response::Status lhs, Response::Status rhs) { return rhs < lhs; }

inline bool operator<=(Response::Status lhs, Response::Status rhs) { return !(rhs < lhs); }

void print(const Response& response, std::istream& s);

void ignore(const Response& response, std::istream& s);

class HttpException : public std::exception {
  public:
    HttpException(Response::Status status, const std::string& reason) : status_(status), reason_(reason) {}
    const char* what() const noexcept override { return reason_.c_str(); }
    Response::Status status() const { return status_; }

  private:
    Response::Status status_;
    std::string reason_;
};

class ResponseHandler {
  public:
    virtual void handleResponse(const Response& response, std::istream& s) = 0;
    virtual void onHttpError(const std::exception&) = 0;
    virtual ~ResponseHandler() = default;
};

class RequestWithResponseHandler : public Request, public ResponseHandler {
  public:
    using Request::Request;
};

class ResponseIgnoringRequest : public RequestWithResponseHandler {
    using RequestWithResponseHandler::RequestWithResponseHandler;
    void handleResponse(const Response& response, std::istream& s) override { ignore(response, s); }

    void onHttpError(const std::exception&) override {}

  private:
};

template <typename Functor>
class RequestWithPolicy : public RequestWithResponseHandler {
  public:
    RequestWithPolicy(Method method, std::string location, Functor f, OnRedirect follow_redirects = OnRedirect::FOLLOW)
        : RequestWithResponseHandler(method, std::move(location), follow_redirects), f_(f) {}
    RequestWithPolicy(std::string location, Functor f) : RequestWithResponseHandler(std::move(location)), f_(f) {}
    void handleResponse(const Response& response, std::istream& s) override { f_(response, s); }

    void onHttpError(const std::exception&) override {}

  private:
    Functor f_;
};

class HTTPSRequestHandler {
  public:
    HTTPSRequestHandler(std::string host, std::uint16_t port);
    virtual ~HTTPSRequestHandler();

    void postRequest(const std::shared_ptr<RequestWithResponseHandler>& request, const std::string& data);
    void postRequest(RequestWithResponseHandler& request, const std::string& data);

    HTTPSRequestHandler(const HTTPSRequestHandler&) = delete;
    HTTPSRequestHandler& operator=(const HTTPSRequestHandler&) = delete;
    HTTPSRequestHandler(HTTPSRequestHandler&&) = default;
    HTTPSRequestHandler& operator=(HTTPSRequestHandler&&) = default;

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

class HTTPSRequestHandlerGroup {
  public:
    HTTPSRequestHandlerGroup(std::size_t n, const std::string& host, std::uint16_t port);
    ~HTTPSRequestHandlerGroup();

    void postRequest(const std::shared_ptr<RequestWithResponseHandler>& request, const std::string& data);

    void postRequest(RequestWithResponseHandler& request, const std::string& data);

    HTTPSRequestHandlerGroup(const HTTPSRequestHandlerGroup&) = delete;
    HTTPSRequestHandlerGroup& operator=(const HTTPSRequestHandlerGroup&) = delete;
    HTTPSRequestHandlerGroup(HTTPSRequestHandlerGroup&&) = default;
    HTTPSRequestHandlerGroup& operator=(HTTPSRequestHandlerGroup&&) = default;

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

std::string simpleGet(const std::string& url);

void simpleGet(const std::string& url, std::ostream& out);
