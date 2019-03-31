#include "HTTPSRequestHandler.h"
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <boost/asio/io_service.hpp>

static std::streamsize to_streamsize(std::size_t n) {
    assert(n <= static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()));
    return static_cast<std::streamsize>(n);
}

static Poco::Net::Context::Ptr createContext() {
    return new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", Poco::Net::Context::VERIFY_RELAXED, 9, true);
}

static Response::Status convert(Poco::Net::HTTPResponse::HTTPStatus status) { return static_cast<Response::Status>(static_cast<int>(status)); }

static Poco::Net::NameValueCollection convert(const NameValueCollection& x) {
     Poco::Net::NameValueCollection y;
     for (const auto& e : x)
        y.add(e.first, e.second);
     return y;
}

static void handleRequestNoRedirect(Poco::Net::HTTPClientSession& client, const Request& request, const std::string& data, NameValueCollection& cookies, std::function<void(const Response& response, std::istream& s)> f) {
    const auto& method = request.getMethod() == Request::Method::GET ? Poco::Net::HTTPRequest::HTTP_GET : Poco::Net::HTTPRequest::HTTP_POST;
    Poco::Net::HTTPRequest pocorequest(method, request.getLocation(), Poco::Net::HTTPMessage::HTTP_1_1);
    pocorequest.setHost(client.getHost());
    pocorequest.add("Accept", "*/*");
    pocorequest.add("User-Agent", "curl/7.44.0");
    if (!data.empty()) {
        pocorequest.setContentLength(to_streamsize(data.size()));
        pocorequest.setContentType("application/x-www-form-urlencoded");
    }

    if (!cookies.empty()) {
        pocorequest.setCookies(convert(cookies));
    }
    //std::cout << pocorequest.getMethod() << " " << pocorequest.getURI() << " " << pocorequest.getVersion() << '\n';
    //for (const auto& header : pocorequest)
    //    std::cout << header.first << ": " << header.second << '\n';
    //std::cout << '\n';
    //std::cout << data << '\n';
    auto& in = client.sendRequest(pocorequest);
    if (!data.empty())
        in << data;
    Poco::Net::HTTPResponse response;
    auto& out = client.receiveResponse(response);
    std::vector<Poco::Net::HTTPCookie> cookieVector;
    response.getCookies(cookieVector);
    // TODO: cookies is basically a multimap. What if we accidentally add a name multiple times?
    for (const auto& cookie : cookieVector)
        cookies[cookie.getName()] = cookie.getValue();
        //cookies.add(cookie.getName(), cookie.getValue());
    //std::cout << response.getStatus() << " " << response.getReason() << '\n';
    //for (const auto& header : response)
    //    std::cout << header.first << ": " << header.second << '\n';
    //std::cout << '\n';
    f(Response(convert(response.getStatus()), response.getReason(), response.begin(), response.end()), out);
    //std::cout << '\n';
}

static void handleRedirect(Poco::Net::HTTPClientSession& client, NameValueCollection& cookies, std::function<void(const Response& response, std::istream& s)> f, const Response& response, std::istream& s) {
    auto status = response.getStatus();
    //std::cout << "handleRedirect, status = " << status << '\n';
    if (status == Response::Status::HTTP_FOUND || status == Response::Status::HTTP_MOVED_PERMANENTLY) {
        //std::cout << "need to handle redirect (status 301 or 302. Ignoring body)\n";
        //std::cout << s.rdbuf();
        s.ignore(std::numeric_limits<std::streamsize>::max());
        if (!response.has("Location")) {
            throw std::runtime_error("Redirect without location");
        }
        //std::cout << "New location: " << response["Location"] << '\n';
        handleRequestNoRedirect(client, Request(response["Location"]), "", cookies, [&] (const auto& response_, auto& s_) { handleRedirect(client, cookies, f, response_, s_); });
        //std::cout << "leaving handleRedirect after handling a redirect\n";
    } else {
        f(response, s);
        //std::cout << "leaving handleRedirect without having to handle a redirect\n";
    }
}

static void handleRequest(Poco::Net::HTTPClientSession& client, const Request& request, const std::string& data, NameValueCollection& cookies, std::function<void(const Response& response, std::istream& s)> f) {
    if (request.getFollowRedirects() == Request::OnRedirect::FOLLOW) {
        handleRequestNoRedirect(client, request, data, cookies, [&] (const auto& response, auto& s) { handleRedirect(client, cookies, f, response, s); });
    } else {
        handleRequestNoRedirect(client, request, data, cookies, f);
    }
}

struct HTTPSRequestHandler::impl {
    impl(std::string host, std::uint16_t port) : session(std::move(host), port, context), work(ioService), thread([&] { ioService.run(); }) {
        session.setKeepAlive(true);
        //session.setTimeout(Timespan(1800,0));
    }

    ~impl() {
        ioService.stop();
        thread.join();
    }

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;
    impl(impl&&) = delete;
    impl& operator=(impl&&) = delete;

    static Poco::Net::Context::Ptr context;
    Poco::Net::HTTPSClientSession session;
    boost::asio::io_service ioService;
    boost::asio::io_service::work work;
    NameValueCollection cookies;
    std::thread thread;
};

HTTPSRequestHandler::HTTPSRequestHandler(std::string host, std::uint16_t port) : pimpl(new impl(std::move(host), port)) {}

HTTPSRequestHandler::~HTTPSRequestHandler() = default;

void HTTPSRequestHandler::postRequest(const std::shared_ptr<RequestWithResponseHandler>& request, const std::string& data) {
    pimpl->ioService.post([&, request, data] {
        try {
            ::handleRequest(pimpl->session, *request, data, pimpl->cookies, [request] (const auto& response, auto& s) {
                if (response.getStatus() >= Response::Status::HTTP_OK && response.getStatus() < Response::Status::HTTP_MULTIPLE_CHOICES)
                    request->handleResponse(response, s);
                else
                    throw HttpException(response.getStatus(), response.getReason());
            });
        } catch (const std::exception& e) {
            request->onHttpError(e);
        }
    });
}

void HTTPSRequestHandler::postRequest(RequestWithResponseHandler& request, const std::string& data) {
    pimpl->ioService.post([&, data] {
        try {
            ::handleRequest(pimpl->session, request, data, pimpl->cookies, [&] (const auto& response, auto& s) {
                if (response.getStatus() >= Response::Status::HTTP_OK && response.getStatus() < Response::Status::HTTP_MULTIPLE_CHOICES)
                    request.handleResponse(response, s);
                else
                    throw HttpException(response.getStatus(), response.getReason());
            });
        } catch (const std::exception& e) {
            request.onHttpError(e);
        }
    });
}

Poco::Net::Context::Ptr HTTPSRequestHandler::impl::context = createContext();

void print(const Response&, std::istream& s) {
    std::cout << s.rdbuf();
}

void ignore(const Response&, std::istream& s) {
    s.ignore(std::numeric_limits<std::streamsize>::max());
}

struct HTTPSRequestHandlerGroup::impl {
public:
    impl(std::size_t n, const std::string& host, std::uint16_t port) : next() {
        handlers.reserve(n);
        for (auto i = 0u; i < n; ++i)
            handlers.emplace_back(host, port);
    }

    void postRequest(const std::shared_ptr<RequestWithResponseHandler>& request, const std::string& data) {
        if (handlers.empty())
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": handlers.empty()");
        handlers[next].postRequest(request, data);
        next = (next + 1) % handlers.size();
    }

    void postRequest(RequestWithResponseHandler& request, const std::string& data) {
        if (handlers.empty())
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": handlers.empty()");
        handlers[next].postRequest(request, data);
        next = (next + 1) % handlers.size();
    }

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;
    impl(impl&&) = default;
    impl& operator=(impl&&) = default;
private:
    std::size_t next;
    std::vector<HTTPSRequestHandler> handlers;
};

HTTPSRequestHandlerGroup::HTTPSRequestHandlerGroup(std::size_t n, const std::string& host, std::uint16_t port) : pimpl(std::make_unique<impl>(n, host, port)) {}
HTTPSRequestHandlerGroup::~HTTPSRequestHandlerGroup() = default;

void HTTPSRequestHandlerGroup::postRequest(const std::shared_ptr<RequestWithResponseHandler>& request, const std::string& data) { pimpl->postRequest(request, data); }

void HTTPSRequestHandlerGroup::postRequest(RequestWithResponseHandler& request, const std::string& data) { pimpl->postRequest(request, data); }

std::string simpleGet(const std::string& url) {
    std::stringstream out;
    simpleGet(url, out);
    return out.str();
}

void simpleGet(const std::string& url, std::ostream& out) {
    Poco::URI uri(url);
    Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), createContext());
    NameValueCollection cookies;
    handleRequest(session, Request(uri.getPathAndQuery()), "", cookies, [&] (const auto& response, auto& stream) {
        if (response.getStatus() >= Response::Status::HTTP_OK && response.getStatus() < Response::Status::HTTP_MULTIPLE_CHOICES)
            out << stream.rdbuf();
        else
            throw HttpException(response.getStatus(), response.getReason());
    });
}
