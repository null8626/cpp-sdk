#pragma once

#include <topgg/topgg.h>

namespace topgg {
/**
 * @brief An API call result that may contain a singular data.
 *
 * @since 2.0.0
 */
template <class T>
class result {
 protected:
  std::variant<exception, request_exception, nlohmann::json::exception, T>
      m_variant{};

  template <class T2>
  inline result(const T2& data) : m_variant(std::in_place_type<T2>, data) {}

 public:
  result() = delete;

  /**
   * @brief Retrieves the contained data with error checking.
   *
   * @return T The contained data.
   * @throw topgg::exception A general Top.gg exception has occurred.
   * @throw topgg::request_exception An HTTP request exception has occurred.
   * @throw nlohmann::json::exception Unable to parse JSON.
   * @since 2.0.0
   */
  const T& get() const {
    if (std::holds_alternative<T>(m_variant)) {
      return std::get<T>(m_variant);
    } else if (std::holds_alternative<request_exception>(m_variant)) {
      throw std::get<request_exception>(m_variant);
    } else if (std::holds_alternative<exception>(m_variant)) {
      throw std::get<exception>(m_variant);
    }

    throw std::get<nlohmann::json::exception>(m_variant);
  }

  friend class base_client;
  friend class base_application_client;
  friend class oauth2_url;
  friend struct project_client;
};

/**
 * @brief An API call result that does not contain any data.
 *
 * @since 2.0.0
 */
class empty_result : private result<std::monostate> {
  template <class T2>
  inline empty_result(const T2& data) : result(data) {}

 public:
  /**
   * @brief Throws an error if the API call failed.
   *
   * @throw topgg::exception A general Top.gg exception has occurred.
   * @throw topgg::request_exception An HTTP request exception has occurred.
   * @throw nlohmann::json::exception Unable to parse JSON.
   * @since 2.0.0
   */
  inline void check() const { result::get(); }

  friend class base_client;
  friend class direct_http_provider;
  friend class http_client;
  friend class oauth2_client;
  friend class oauth2_http_provider;
};

/**
 * @brief An API call result that may contain paginated data.
 *
 * @since 2.0.0
 */
template <class T>
class paginated_result
    : private result<std::pair<std::vector<T>, std::string>> {
  using base_result = result<std::pair<std::vector<T>, std::string>>;

  template <class T2>
  inline paginated_result(const T2& data) : base_result(data) {}

  static inline paginated_result<T> from_array(const char* key,
                                               const nlohmann::json& j) {
    std::vector<T> data{};

    for (const auto& element : j[key]) {
      data.push_back(element);
    }

    std::string cursor{};

    if (j.contains("cursor")) {
      cursor = j["cursor"].template get<std::string>();
    }

    return std::make_pair(data, cursor);
  }

  static inline constexpr std::pair<std::vector<T>, std::string> empty() {
    return std::make_pair(std::vector<T>{}, std::string{});
  }

  inline std::string cursor() const {
    return std::get<std::pair<std::vector<T>, std::string>>(this->m_variant)
        .second;
  }

 public:
  paginated_result() = delete;

  /**
   * @brief Whether this is the last page.
   *
   * @return bool Whether this is the last page.
   * @since 2.0.0
   */
  inline bool is_last() const {
    const auto& pair{
        std::get<std::pair<std::vector<T>, std::string>>(this->m_variant)};

    return pair.second.empty() || pair.first.empty();
  }

  /**
   * @brief Retrieves the contained data with error checking.
   *
   * @return std::vector<T> The contained data.
   * @throw topgg::exception A general Top.gg exception has occurred.
   * @throw topgg::request_exception An HTTP request exception has occurred.
   * @throw nlohmann::json::exception Unable to parse JSON.
   * @since 2.0.0
   */
  inline const std::vector<T>& get() const { return base_result::get().first; }

  friend class base_client;
  friend class http_client;
};
};  // namespace topgg