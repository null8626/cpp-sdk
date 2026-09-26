#pragma once

#include <nlohmann/json.hpp>
#include <topgg/exception.h>
#include <optional>
#include <variant>
#include <string>
#include <vector>


namespace topgg {
  template<class T>
  class paginated_result;
  
  template<class T>
  class result {
    std::variant<exception, http_exception, nlohmann::json::parse_error, T> m_variant{};

    template<typename T2>
    inline result(const T2& data): m_variant(std::in_place_type<T2>, data) {}

  public:
    result() = delete;

    const T& get() const {
      if (std::holds_alternative<T>(m_variant)) {
        return std::get<T>(m_variant);
      } else if (std::holds_alternative<http_exception>(m_variant)) {
        throw std::get<http_exception>(m_variant);
      } else if (std::holds_alternative<exception>(m_variant)) {
        throw std::get<exception>(m_variant);
      }

      throw std::get<nlohmann::json::parse_error>(m_variant);
    }

    friend class client;
    template<class T2>
    friend class paginated_result;
  };

  using empty_result = result<std::monostate>;

  class client;

  template<class T>
  class paginated_result: private result<std::pair<std::vector<T>, std::optional<std::string>>> {
    template<class T2>
    inline paginated_result(const T2& data): result(data) {}

    static inline paginated_result<T> from_array(const char* key, const nlohmann::json& j) {
      std::vector<T> data{};

      for (const auto& element: j[key]) {
        data.push_back(element);
      }

      return std::make_pair(data, j.contains("cursor") ? std::optional{j["cursor"].template get<std::string>()} : std::nullopt);
    }

    static inline std::pair<std::vector<T>, std::optional<std::string>> empty() {
      return std::make_pair(std::vector<T>{}, std::nullopt);
    }

    inline const std::optional<std::string>& cursor() const {
      return std::holds_alternative<std::pair<std::vector<T>, std::optional<std::string>>>(m_variant) ?
        std::get<std::pair<std::vector<T>, std::optional<std::string>>>(m_variant).second :
        std::nullopt;
    }

  public:
    paginated_result() = delete;

    inline const std::vector<T>& get() const {
      return result::get().first;
    }

    friend class client;
  };

  enum project_platform {
    pp_discord,
    pp_roblox,
  };

  enum project_type {
    pt_bot,
    pt_server,
    pt_game,
  };

  class partial_project {
    partial_project(const nlohmann::json& j);

  public:
    std::string id{};
    std::string platform_id{};
    std::string name{};
    project_platform platform{};
    project_type type{};

    friend class client;
    friend class paginated_result<partial_project>;
  };
  
};