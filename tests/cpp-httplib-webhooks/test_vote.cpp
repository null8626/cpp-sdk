#include <topgg/webhooks/cpp-httplib.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

template<class T>
using cpp_httplib_webhook = topgg::webhook::cpp_httplib<T>;
using topgg::webhook::vote;

class my_vote_listener: public cpp_httplib_webhook<vote> {
public:
  inline my_vote_listener(const std::string& authorization): cpp_httplib_webhook<vote>(authorization) {}

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

  httplib::Server server{};
  my_vote_listener webhook{authorization};

  server.Post("/votes", webhook.endpoint());

  std::thread server_thread{[&server]() {
    server.listen("localhost", 8080);
  }};

  std::this_thread::sleep_for(std::chrono::seconds{5});

  httplib::Client client{"http://localhost:8080"};

  const httplib::Headers headers = {
    { "Authorization", authorization },
    { "Content-Type", "application/json" }
  };

  const auto json{R"({"bot":"12345","user":"12345","isWeekend":true,"type":"test"})"};

  const auto response{client.Post("/votes", headers, json, "application/json")};

  if (response && response->status == 204) {
    std::cout << "ok" << std::endl;
  } else {
    std::cerr << "failed" << std::endl;
  }

  server.stop();
  server_thread.join();

  return 0;
}