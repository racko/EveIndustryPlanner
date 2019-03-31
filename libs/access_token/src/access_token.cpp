#include "access_token.h"
#include "HTTPSRequestHandler.h"
#include <iostream>
#include <sstream>
#include <boost/regex.hpp>

using namespace boost;

namespace {
    struct workspace {
        NameValueCollection cookies;
        HTTPSRequestHandler request_handler; // TODO: replace with "sequential" request handler to be able to deal with errors
        workspace() : request_handler("login.eveonline.com", 443) {}

        void step1() {
            auto path = "/oauth/authorize/?response_type=token&client_id=6207584877514be8b23ab78a6163bb82&scope=publicData&redirect_uri=https://racko.pw:51230";
            ResponseIgnoringRequest request1(Request::Method::POST, path);
            request_handler.postRequest(request1, "");
        }

        void step2() {
            auto path =  "/Account/LogOn?ReturnUrl=%2Foauth%2Fauthorize%2F%3Fresponse_type%3Dtoken%26client_id%3D6207584877514be8b23ab78a6163bb82%26scope%3DpublicData%26redirect_uri%3Dhttps%3A%2F%2Fracko.pw%3A51230";
            ResponseIgnoringRequest request2(Request::Method::POST, path);
            static const std::string credentials = "UserName=drunkenmasta1&Password=XxdzqdzEuQX5Gyrx0q68";
            request_handler.postRequest(request2, credentials);
        }

        std::string step3() {
            auto path =  "/Account/Authenticator?ReturnUrl=%2Foauth%2Fauthorize%2F%3Fresponse_type%3Dtoken%26client_id%3D6207584877514be8b23ab78a6163bb82%26scope%3DpublicData%26redirect_uri%3Dhttps%3A%2F%2Fracko.pw%3A51230";
            std::string verificationCode;
            std::cout << "Verification Code: " << std::flush;
            std::cin >> verificationCode;
            const std::string challenge = "Challenge=" + verificationCode;

            std::string requestVerificationToken;
            auto policy = [&] (const auto&, auto& s) {
                std::stringstream body;
                body << s.rdbuf();
                //std::cout << body.str() << '\n';
                static const regex r(".*<input name=\"__RequestVerificationToken\" type=\"hidden\" value=\"([^\"]*)\" />.*");
                cmatch matches;
                if (regex_match(body.str().c_str(), matches, r))
                    requestVerificationToken = matches[1];
                else
                    throw std::runtime_error("Failed to parse request verification token");
            };
            RequestWithPolicy request3(Request::Method::POST, path, policy);
            request_handler.postRequest(request3, challenge);
            std::cout << "request verification token: " << requestVerificationToken << '\n';
            return requestVerificationToken;
        }

        std::string step4(const std::string& requestVerificationToken) {
            auto path = "/oauth/authorize/?response_type=token&amp;client_id=6207584877514be8b23ab78a6163bb82&amp;scope=publicData&amp;redirect_uri=https://racko.pw:51230";
            const std::string data = "ClientIdentifier=6207584877514be8b23ab78a6163bb82&RedirectUri=https://racko.pw:51230&State=&Scope=publicData&ResponseType=token&__RequestVerificationToken=" + requestVerificationToken + "&CharacterId=1679728731";
            std::string access_token;
            auto policy = [&] (const auto& response, auto&) {
                static const regex r(".*#access_token=([^&]*)&.*");
                cmatch matches;
                if (regex_match(response["Location"].c_str(), matches, r))
                    access_token = matches[1];
                else
                    throw std::runtime_error("Failed to parse access token");
            };
            RequestWithPolicy request4(Request::Method::POST, path, policy, Request::OnRedirect::DONT_FOLLOW); // don't follow: We need to read the target location below
            request_handler.postRequest(request4, data);

            return access_token;
        }

        std::string getAccessToken() {
            step1();
            step2();
            return step4(step3());
        }
    };
}
std::string getAccessToken() {
    workspace ws;
    return ws.getAccessToken();
}

