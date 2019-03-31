#define BOOST_NETWORK_ENABLE_HTTPS
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri/uri_io.hpp>

using namespace boost::network;
using http_client = http::client;

class CookieJar {
public:
    void toCookieJar(const http_client::response& response) {
        for (const auto& header : response.headers()) {
            if (header.first == "Set-Cookie") {
                parseValue(header.second);
            }
        }
    }

    void fromCookieJar(http_client::request& request) const {
        if (cookies.empty())
            return;
        auto cookie = cookies.begin();
        std::stringstream cookieStr;
        cookieStr << cookie->first << "=" << cookie->second;
        for (++cookie; cookie != cookies.end(); ++cookie)
            cookieStr << "; " << cookie->first << "=" << cookie->second;
        request << header("Cookie", cookieStr.str());
    }
private:
    void parseValue(const std::string& value) {
        auto eql = value.find('=');
        if (eql == std::string::npos)
            throw std::runtime_error("Failed to parse Cookie: Missing '='");
        auto end = value.find(';', eql + 1);
        if (end == std::string::npos)
            throw std::runtime_error("Failed to parse Cookie: Missing ';'");
        std::string k(value.data(), eql);
        std::string v(value.data() + eql + 1, end - eql - 1);
        std::cout << "Adding Cookie: " << k << "=" << v << '\n';
        cookies[k] = v;
    }
    std::unordered_map<std::string, std::string> cookies;
};

template<typename T>
void printHeaders(const T& x) {
    for (const auto& e : x.headers())
        std::cout << e.first << ": " << e.second << '\n';
}

template<typename Functor>
http_client::response doRequestNoFollow(http_client& client, http_client::request& request, CookieJar& cookieJar, Functor f) {
    //cookieJar.fromCookieJar(request);
    auto response = f(client, request);
    //std::cout << '\n';
    //auto status = response.status();
    //std::cout << "Status: " << status << "(" << response.status_message() << ")\n";
    //printHeaders(response);
    //std::cout << '\n';
    //cookieJar.toCookieJar(response);
    //std::cout << '\n';
    //std::cout << response.body() << '\n';
    //std::cout << '\n';
    return response;
}
http_client::response doGet(http_client& client, http_client::request& request) {
    //request << header("Host", request.host());
    //request << header("Accept", "*/*");
    //request << header("User-Agent", "industryapp/0.0.0");
    //const auto& uri = request.uri();
    //std::cout << "GET " << uri.path() << '?' << uri.query() << '\n';
    //printHeaders(request);
    return client.get(request);
}

http_client::response doPost(http_client& client, http_client::request& request) {
    //request << header("Host", request.host());
    //request << header("Content-Type", "application/x-www-form-urlencoded");
    //request << header("Accept", "*/*");
    //request << header("User-Agent", "industryapp/0.0.0");
    //const auto& uri = request.uri();
    //std::cout << "POST " << uri.path() << '?' << uri.query() << '\n';
    //printHeaders(request);
    return client.post(request);
}


template<typename Functor>
http_client::response doRequest(http_client& client, http_client::request& request, CookieJar& cookieJar, Functor f) {
    auto response = doRequestNoFollow(client, request, cookieJar, f);
    //std::cout << '\n';
    //std::string location;
    //const auto& uri = request.uri();
    //uri::uri base_uri = uri.scheme() + "://" + uri.host();
    //auto status = response.status();
    //while (status == 302 || status == 301) {
    //    const auto& headers = response.headers();
    //    auto locationIt = headers.find("Location");
    //    if (locationIt != headers.end()) {
    //        std::cout << "New location: " << locationIt->second << '\n';
    //        location = locationIt->second;
    //    } else
    //        throw std::runtime_error("Redirect without location");
    //    uri::uri uri;
    //    uri << base_uri << uri::path(location);
    //    http_client::request request(uri);
    //    response = doRequestNoFollow(client, request, cookieJar, &doGet);
    //    status = response.status();
    //}
    return response;
}

http_client::response get(http_client& client, http_client::request& request, CookieJar& cookieJar) {
    return doRequest(client, request, cookieJar, doGet);
}

http_client::response post(http_client& client, http_client::request& request, CookieJar& cookieJar) {
    return doRequest(client, request, cookieJar, doPost);
}

std::string getAccessToken(CookieJar& cookieJar) {
    //using http_client = http::basic_client<http::tags::http_keepalive_8bit_udp_resolve, 1, 1>;
    http_client::options options;
    //options.follow_redirects(true);
    http_client client(options);
    uri::uri base_uri("https://login.eveonline.com");
    
    {
        uri::uri uri;
        uri << base_uri << uri::path("/oauth/authorize/?response_type=token&client_id=6207584877514be8b23ab78a6163bb82&scope=publicData&redirect_uri=https://racko.pw:51230");
        http_client::request request(uri);
        auto response = get(client, request, cookieJar);
    }
    {
        uri::uri uri("https://login.eveonline.com/Account/LogOn?ReturnUrl=%2Foauth%2Fauthorize%2F%3Fresponse_type%3Dtoken%26client_id%3D6207584877514be8b23ab78a6163bb82%26scope%3DpublicData%26redirect_uri%3Dhttps%3A%2F%2Fracko.pw%3A51230");
        http_client::request request(uri);
        static const std::string credentials = "UserName=drunkenmasta1&Password=XxdzqdzEuQX5Gyrx0q68";
        request << header("Content-Length", std::to_string(credentials.length()));
        request << body(credentials);
        auto response = client.post(request);
        //auto response = post(client, request, cookieJar);
    }
    //std::string requestVerificationToken;
    //{
    //    std::string verificationCode;
    //    std::cout << "Verification Code: " << std::flush;
    //    std::cin >> verificationCode;
    //    uri::uri uri("https://login.eveonline.com/Account/Authenticator?ReturnUrl=%2Foauth%2Fauthorize%2F%3Fresponse_type%3Dtoken%26client_id%3D6207584877514be8b23ab78a6163bb82%26scope%3DpublicData%26redirect_uri%3Dhttps%3A%2F%2Fracko.pw%3A51230");
    //    http_client::request request(uri);
    //    const std::string challenge = "Challenge=" + verificationCode;
    //    request << header("Content-Length", std::to_string(challenge.length()));
    //    request << body(challenge);
    //    auto response = post(client, request, cookieJar);

    //    static const regex r(".*<input name=\"__RequestVerificationToken\" type=\"hidden\" value=\"([^\"]*)\" />.*");
    //    cmatch matches;
    //    if (regex_match(response.body().c_str(), matches, r))
    //        requestVerificationToken = matches[1];
    //    else
    //        throw std::runtime_error("Failed to parse request verification token");
    //    std::cout << "request verification token: " << requestVerificationToken << '\n';
    //}
    //options.follow_redirects(false);
    //client = http_client(options);
    //{
    //    uri::uri uri("https://login.eveonline.com/oauth/authorize/?response_type=token&amp;client_id=6207584877514be8b23ab78a6163bb82&amp;scope=publicData&amp;redirect_uri=https://racko.pw:51230");
    //    http_client::request request(uri);
    //    const std::string data = "ClientIdentifier=6207584877514be8b23ab78a6163bb82&RedirectUri=https://racko.pw:51230&State=&Scope=publicData&ResponseType=token&__RequestVerificationToken=" + requestVerificationToken + "&CharacterId=1679728731";
    //    request << header("Content-Length", std::to_string(data.length()));
    //    request << body(data);
    //    auto response = post(client, request, cookieJar);
    //    static const regex r(".*#access_token=([^&]*)&.*");
    //    cmatch matches;
    //    const auto& headers = response.headers();
    //    auto location = headers.find("Location");
    //    if (location != headers.end() && regex_match(location->second.c_str(), matches, r))
    //        return matches[1];
    //    else
    //        throw std::runtime_error("Failed to parse access token");
    //}
    return "";
}

int main() {}
