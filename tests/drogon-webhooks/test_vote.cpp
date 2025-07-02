#include <topgg/webhooks/drogon.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

template<class T>
using drogon_webhook = topgg::webhook::drogon<T>;
using topgg::webhook::vote;

class my_vote_listener: public ::drogon::HttpSimpleController<my_vote_listener, false>, public drogon_webhook<vote> {
public:
  inline my_vote_listener(const std::string& authorization): drogon_webhook<vote>(authorization) {}

  TOPGG_DROGON_WEBHOOK();

  PATH_LIST_BEGIN
  PATH_ADD("/votes", ::drogon::Post);
  PATH_LIST_END

  void callback(const vote& v) override {
    std::cout << "A user with the ID of " << v.voter_id << " has voted us on Top.gg!" << std::endl;
  }
};

int main() {
  const auto authorization{std::getenv("MY_TOPGG_WEBHOOK_SECRET")};

  if (authorization == nullptr) {
    std::cerr << "error: missing MY_TOPGG_WEBHOOK_SECRET environment variable" << std::endl;
    return 1;
  }

  auto& app{drogon::app()};

  app.registerController(std::make_shared<my_vote_listener>(authorization));

  std::thread server_thread([&app]() {
    app.addListener("127.0.0.1", 8080);
    app.run();
  });

  std::this_thread::sleep_for(std::chrono::seconds{5});

  const auto client{drogon::HttpClient::newHttpClient("http://127.0.0.1:8080")};

  Json::Value body{};

  body["bot"] = "12345";
  body["user"] = "12345";
  body["isWeekend"] = true;
  body["type"] = "test";

  const auto request{drogon::HttpRequest::newHttpJsonRequest(body)};

  request->setMethod(drogon::Post);
  request->setPath("/votes");
  request->addHeader("Authorization", authorization);

  client->sendRequest(request, [&app](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
    const auto status_code{response->statusCode()};

    if (result == drogon::ReqResult::Ok && status_code == drogon::k204NoContent) {
      std::cout << "ok" << std::endl;
    } else {
      std::cerr << "failed (" << static_cast<int>(status_code) << ")" << std::endl;
    }

    app.quit();
  });

  server_thread.join();

  return 0;
}