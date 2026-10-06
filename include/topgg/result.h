#pragma once

#include <nlohmann/json.hpp>
#include <topgg/topgg.h>
#include <optional>
#include <cstdint>
#include <variant>
#include <string>
#include <vector>


namespace topgg {
  class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  class oauth2_client;
#endif

  template<class T>
  class result {
  protected:
    std::variant<exception, http_exception, nlohmann::json::exception, T> m_variant{};

    template<class T2>
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

      throw std::get<nlohmann::json::exception>(m_variant);
    }

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
    friend class oauth2;
#endif
  };

  class empty_result: private result<std::monostate> {
    template<class T2>
    inline empty_result(const T2& data): result(data) {}

  public:
    inline void check() const {
      result::get();
    }

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };

  template<class T>
  class paginated_result: private result<std::pair<std::vector<T>, std::optional<std::string>>> {
    using base_result = result<std::pair<std::vector<T>, std::optional<std::string>>>;

    template<class T2>
    inline paginated_result(const T2& data): base_result(data) {}

    static inline paginated_result<T> from_array(const char* key, const nlohmann::json& j) {
      std::vector<T> data{};

      for (const auto& element: j[key]) {
        data.push_back(element);
      }

      return std::make_pair(data, j.contains("cursor") ? std::optional{j["cursor"].template get<std::string>()} : std::nullopt);
    }

    static inline constexpr std::pair<std::vector<T>, std::optional<std::string>> empty() {
      return std::make_pair(std::vector<T>{}, std::nullopt);
    }

    inline std::optional<std::string> cursor() const {
      return std::holds_alternative<std::pair<std::vector<T>, std::optional<std::string>>>(this->m_variant) ?
        std::get<std::pair<std::vector<T>, std::optional<std::string>>>(this->m_variant).second :
        std::nullopt;
    }

  public:
    paginated_result() = delete;

    inline const std::vector<T>& get() const {
      return base_result::get().first;
    }

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };
};