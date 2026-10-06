#pragma once

#include <nlohmann/json.hpp>
#include <topgg/topgg.h>
#include <unordered_map>
#include <variant>
#include <string>


namespace topgg::webhooks {
  class vote_create;
  class test;
  class integration_create;
  class integration_delete;

  using payload = std::variant<vote_create, test, integration_create, integration_delete>;

  class project {
    project(const nlohmann::json& j);

  public:
    project() = delete;

    std::string id{};
    std::string platform_id{};
    topgg::platform platform{};
    project_type type{};

    friend class vote_create;
    friend class test;
    friend class integration_create;
  };

  class user {
    user(const nlohmann::json& j);

  public:
    user() = delete;

    std::string id{};
    std::string platform_id{};
    std::string name{};
    std::string avatar{};

    friend class vote_create;
    friend class test;
    friend class integration_create;
  };

  class vote_create {
    vote_create(const nlohmann::json& j);

  public:
    vote_create() = delete;

    std::string id{};
    uint16_t weight{};
    time_t created_at{};
    time_t expires_at{};
    webhooks::project project;
    webhooks::user user;
    std::unordered_map<std::string, std::string> query{};

    friend webhooks::payload parse(const std::string& body);
  };

  class test {
    test(const nlohmann::json& j);

  public:
    test() = delete;

    webhooks::project project;
    webhooks::user user;

    friend webhooks::payload parse(const std::string& body);
  };

  class integration_create {
    integration_create(const nlohmann::json& j);

  public:
    integration_create() = delete;

    std::string connection_id{};
    std::string webhook_secret{};
    webhooks::project project;
    webhooks::user user;

    friend webhooks::payload parse(const std::string& body);
  };

  class integration_delete {
    integration_delete(const nlohmann::json& j);

  public:
    integration_delete() = delete;

    std::string connection_id{};

    friend webhooks::payload parse(const std::string& body);
  };
};