#include <topgg/result.h>


topgg::partial_project::partial_project(const nlohmann::json& j) {
  id = j["id"].template get<std::string>();
  platform_id = j["platform_id"].template get<std::string>();
  name = j["name"].template get<std::string>();

  const auto platform_{j["platform"].template get<std::string>()};

  if (platform_ == "discord") {
    platform = topgg::project_platform::pp_discord;
  } else {
    platform = topgg::project_platform::pp_roblox;
  }

  const auto type_{j["platform_type"].template get<std::string>()};

  if (type_ == "bot") {
    type = topgg::project_type::pt_bot;
  } else if (type_ == "server") {
    type = topgg::project_type::pt_server;
  } else {
    type = topgg::project_type::pt_game;
  }
}