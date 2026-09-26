#include <topgg/client.h>
#include <iostream>

void topgg::client::internal_get_projects(const std::string& path, const topgg::paginated_callback<topgg::partial_project>& callback, const bool defer) {
  fetch("GET", path, [callback](const http_response& response) {
    if (std::holds_alternative<topgg::exception>(response)) {
      callback(std::get<topgg::exception>(response));
    } else {
      try {
        const auto& response_pair{std::get<std::pair<uint16_t, std::string_view>>(response)};

        if (response_pair.first >= 400) {
          callback(topgg::http_exception{response_pair});
        } else {
          callback(topgg::paginated_result<topgg::partial_project>::from_array("projects", nlohmann::json::parse(response_pair.second)));
        }
      } catch (const nlohmann::json::parse_error& error) {
        callback(error);
      }
    }
  }, defer);
}

void topgg::client::get_projects(const topgg::paginated_result<topgg::partial_project>& cursor_, const topgg::paginated_callback<topgg::partial_project>& callback, const bool defer) {
  const auto cursor{cursor_.cursor()};
  
  if (cursor.has_value()) {
    internal_get_projects("/projects?cursor=" + cursor.value(), callback, defer);
  } else {
    callback(topgg::paginated_result<topgg::partial_project>::empty());
  }
}