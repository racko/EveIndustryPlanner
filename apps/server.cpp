#include "libs/QueryRunner/QueryRunner.h"
#include "libs/Semaphore/Semaphore.h"
#include "libs/sqlite-util/sqlite.h"
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/URI.h>
#include <csignal>
#include <iostream>
#include <json/writer.h>

using namespace Poco;
using namespace Poco::Net;

class AccountLister : public HTTPRequestHandler {
  private:
    QueryRunnerGroup& queryRunner;

  public:
    AccountLister(QueryRunnerGroup& q) : queryRunner(q) {}
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        response.set("Access-Control-Allow-Origin", "*");
        std::cout << "[" << request.clientAddress().toString() << "] " << request.getMethod() << " " << request.getURI()
                  << " " << request.getVersion() << '\n';
        for (const auto& header : request)
            std::cout << header.first << ": " << header.second << '\n';
        std::cout << '\n';
        Semaphore semaphore;
        queryRunner.postQuery([&](const sqlite::dbPtr&, const sqlite::stmtPtr& stmt) {
            Json::Value root(Json::objectValue);
            auto& accounts = root["accounts"] = Json::Value(Json::arrayValue);

            try {
                auto count = 0u;
                int rc;
                for (rc = sqlite::step(stmt); rc == sqlite::ROW; rc = sqlite::step(stmt)) {
                    auto id = sqlite::column<int>(stmt, 0);
                    auto descr = sqlite::column<std::string>(stmt, 1);
                    Json::Value account(Json::objectValue);
                    account["id"] = id;
                    account["description"] = descr;
                    accounts.append(account);
                    ++count;
                }
                std::cout << "Have " << count << " accounts.\n";
                std::cout << "result code: " << rc << " (" << sqlite::errstr(rc) << ")\n";
                response.setStatusAndReason(HTTPResponse::HTTPStatus::HTTP_OK);
                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                auto writer = builder.newStreamWriter();
                auto& stream = response.send();
                writer->write(root, &stream);
                stream << "\n";
            } catch (const std::exception& e) {
                std::cout << "Exception: " << e.what() << '\n';
                response.setStatusAndReason(HTTPResponse::HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
                response.send();
            }
            semaphore.release();
        });
        semaphore.acquire();
    }
};

class AccountAdder : public HTTPRequestHandler {
  private:
    QueryRunner& queryRunner;

  public:
    AccountAdder(QueryRunner& q) : queryRunner(q) {}
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        response.set("Access-Control-Allow-Origin", "*");
        std::cout << "[" << request.clientAddress().toString() << "] " << request.getMethod() << " " << request.getURI()
                  << " " << request.getVersion() << '\n';
        for (const auto& header : request)
            std::cout << header.first << ": " << header.second << '\n';
        std::cout << '\n';
        HTMLForm form(request, request.stream());
        auto nameIt = form.find("name");
        if (nameIt == form.end()) {
            response.setStatusAndReason(HTTPResponse::HTTPStatus::HTTP_BAD_REQUEST);

            std::cout << "missing account name\n";
            response.send() << "Missing account name\n";
            return;
        }
        const auto& name = nameIt->second;
        Semaphore semaphore;
        queryRunner.postQuery([&](const sqlite::dbPtr& db, const sqlite::stmtPtr& stmt) {
            sqlite::bind(stmt, 1, name);
            try {
                sqlite::step(stmt);
                auto id = sqlite::last_insert_rowid(db);
                response.setStatusAndReason(HTTPResponse::HTTPStatus::HTTP_CREATED);
                std::cout << "Account added.\nName: \"" << name << "\"\nid: " << id << "\n\n";
                response.send() << "Account added.\nName: \"" << name << "\"\nid: " << id << "\n";
            } catch (const std::exception& e) {
                std::cout << "Exception: " << e.what() << '\n';
                response.setStatusAndReason(HTTPResponse::HTTPStatus::HTTP_CONFLICT);
                response.send() << "Account \"" << name << "\" already exists.\n";
            }
            semaphore.release();
        });
        semaphore.acquire();
    }
};

class DefaultHandler : public HTTPRequestHandler {
  private:
  public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        response.set("Access-Control-Allow-Origin", "*");
        std::cout << "[" << request.clientAddress().toString() << "] " << request.getMethod() << " " << request.getURI()
                  << " " << request.getVersion() << '\n';
        for (const auto& header : request)
            std::cout << header.first << ": " << header.second << '\n';
        std::cout << '\n';
        response.setStatusAndReason(HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
        response.send();
    }
};

class RequestHandlerFactory : public HTTPRequestHandlerFactory {
  private:
    QueryRunner accountAdder;
    QueryRunnerGroup accountLister;

  public:
    RequestHandlerFactory(const char* path)
        : accountAdder(path, "insert into accounts(description) values(?)"),
          accountLister(1, path, "select * from accounts") {}
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) override {
        auto path = URI(request.getURI()).getPath();
        if (path == "/addAccount")
            return new AccountAdder(accountAdder);
        else if (path == "/listAccounts")
            return new AccountLister(accountLister);
        else
            return new DefaultHandler();
    }
};

Semaphore interrupted;
void sigint_handler(int) { interrupted.release(); }

int main() {
    HTTPServer server(new RequestHandlerFactory("market_history.db"), 8081, new HTTPServerParams());
    server.start();
    std::signal(SIGINT, &sigint_handler);
    interrupted.acquire();
    server.stopAll();
    return 0;
}
