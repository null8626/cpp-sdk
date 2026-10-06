#pragma once

#include <nlohmann/json.hpp>
#include <type_traits>
#include <string>
#include <ctime>


namespace topgg {
  class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
  class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
  class oauth2_client;
#endif
  template<class T>
  class paginated_result;

  enum platform {
    p_discord,
    p_roblox,
  };

  enum project_type {
    pt_bot,
    pt_server,
    pt_game,
  };

  class partial_project {
  protected:
    partial_project(const nlohmann::json& j);

  public:
    partial_project() = delete;

    std::string id{};
    std::string platform_id{};
    std::string name{};
    topgg::platform platform{};
    project_type type{};

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
    friend class paginated_result<partial_project>;
  };

  class project {
    project(const nlohmann::json& j);

  public:
    project() = delete;

    std::string id{};
    std::string name{};
    topgg::platform platform{};
    project_type type{};
    std::string headline{};
    std::vector<std::string> tags{};
    uint64_t current_votes{};
    uint64_t total_votes{};
    float review_score{};
    uint64_t review_count{};

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
    friend class paginated_result<project>;
  };

  enum locale {
    l_english,
    l_german,
    l_french,
    l_portuguese,
    l_turkish,
    l_hindi,
    l_japanese,
    l_arabic,
    l_dutch,
    l_korean,
    l_italian,
    l_spanish,
    l_russian,
    l_ukrainian,
    l_vietnamese,
    l_chinese_simplified,
  };

  class localized_string {
    nlohmann::json m_json{};

    void enforce_constraints(const uint16_t minimum, const uint16_t maximum) const;

  public:
    void set(const locale& locale_, const std::string& text);

    friend class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
    friend class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };

  enum announcement_category {
    ac_announcement,
    ac_event,
    ac_new_feature,
  };

  class metrics {
    virtual nlohmann::json to_json() const = 0;

    friend class base_client;
#ifndef TOPGG_OAUTH2_ACCESS_TOKENS_ONLY
    friend class client;
#endif
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };

  template<class T>
  class timestamped_metrics;

  class discord_bot_metrics: public metrics {
    std::optional<uint64_t> m_server_count{};
    std::optional<uint64_t> m_shard_count{};

    inline constexpr discord_bot_metrics(const std::pair<std::optional<uint64_t>, std::optional<uint64_t>>& pair): m_server_count(pair.first), m_shard_count(pair.second) {}

    nlohmann::json to_json() const override;

  public:
    discord_bot_metrics() = delete;

    inline constexpr discord_bot_metrics(const uint64_t server_count_, const uint64_t shard_count_): m_server_count(server_count_), m_shard_count(shard_count_) {}

    static inline constexpr discord_bot_metrics server_count(const uint64_t server_count_) noexcept {
      return {std::make_pair(server_count_, std::nullopt)};
    }

    static inline constexpr discord_bot_metrics shard_count(const uint64_t shard_count_) noexcept {
      return {std::make_pair(std::nullopt, shard_count_)};
    }

    friend class timestamped_metrics<discord_bot_metrics>;
  };

  class discord_server_metrics: public metrics {
    std::optional<uint64_t> m_total_member_count{};
    std::optional<uint64_t> m_online_member_count{};

    inline constexpr discord_server_metrics(const std::pair<std::optional<uint64_t>, std::optional<uint64_t>>& pair): m_total_member_count(pair.first), m_online_member_count(pair.second) {}

    nlohmann::json to_json() const override;

  public:
    discord_server_metrics() = delete;

    inline constexpr discord_server_metrics(const uint64_t total_member_count_, const uint64_t online_member_count_): m_total_member_count(total_member_count_), m_online_member_count(online_member_count_) {}

    static inline constexpr discord_server_metrics total_member_count(const uint64_t total_member_count_) noexcept {
      return {std::make_pair(total_member_count_, std::nullopt)};
    }

    static inline constexpr discord_server_metrics online_member_count(const uint64_t online_member_count_) noexcept {
      return {std::make_pair(std::nullopt, online_member_count_)};
    }

    friend class timestamped_metrics<discord_server_metrics>;
  };

  class roblox_metrics: public metrics {
    uint64_t m_player_count{};

    nlohmann::json to_json() const override;

  public:
    roblox_metrics() = delete;

    inline constexpr roblox_metrics(const uint64_t player_count): m_player_count(player_count) {}

    friend class timestamped_metrics<roblox_metrics>;
  };

  std::string _to_time_string(const time_t timestamp);

  template<class T>
  class timestamped_metrics {
    static_assert(std::is_base_of_v<metrics, T>, "timestamped_metrics class template must be a child of metrics");

    nlohmann::json m_json{};

  public:
    inline timestamped_metrics(): m_json(nlohmann::json::array()) {}

    void add(const time_t timestamp, const T& metrics_) {
      nlohmann::json json{};

      json["timestamp"] = _to_time_string(timestamp);
      json["metrics"] = metrics_.to_json();

      m_json.push_back(json);
    }

    inline void add(const T& metrics_) {
      add(time(nullptr), metrics_);
    }

    inline void clear() {
      m_json.clear();
    }

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };

  class partial_vote {
  protected:
    partial_vote(const nlohmann::json& j);

  public:
    partial_vote() = delete;

    uint16_t weight{};
    time_t created_at{};
    time_t expires_at{};

    inline bool has_expired() const noexcept {
      return time(nullptr) >= expires_at;
    }

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
  };

  enum user_source {
    us_topgg,
    us_discord,
  };

  class vote: public partial_vote {
    vote(const nlohmann::json& j);

  public:
    vote() = delete;

    std::string user_id{};
    std::string platform_id{};

    friend class base_client;
#ifndef TOPGG_PROJECT_TOKENS_ONLY
    friend class oauth2_client;
#endif
    friend class paginated_result<vote>;
  };

#ifndef TOPGG_PROJECT_TOKENS_ONLY
  class integration {
    integration(const nlohmann::json& j);

  public:
    integration() = delete;

    std::string id{};
    std::string name{};
    std::string description{};
    std::string icon_url{};
    bool connected{};

    friend class base_client;
    friend class oauth2_client;
  };

  struct base_webhook {
    std::string label{};
    std::string url{};

    base_webhook() = default;

  protected:
    base_webhook(const nlohmann::json& j);

    friend class oauth2_client;
  };

  class webhook: public base_webhook {
    webhook(const nlohmann::json& j);

  public:
    webhook() = delete;

    std::string id{};

    friend class base_client;
    friend class oauth2_client;
  };

  struct user_connection {
    topgg::platform platform{};
    std::string id{};
  };

  class user {
    user(const nlohmann::json& j);

  public:
    user() = delete;

    std::string id{};
    std::string username{};
    std::optional<std::string> avatar{std::nullopt};
    std::vector<user_connection> connections{};

    friend class base_client;
    friend class oauth2_client;
  };

  enum review_status {
    rs_draft,
    rs_in_review,
    rs_approved,
  };

  class user_project: public partial_project {
    user_project(const nlohmann::json& j);

  public:
    user_project() = delete;

    std::string headline{};
    topgg::review_status review_status{};

    friend class base_client;
    friend class oauth2_client;
  };

  struct project_submission {
    project_submission() = delete;

    inline project_submission(const platform platform_, const project_type type_, const std::string& platform_id_, const std::string& headline_, const std::string& page_content_): platform(platform_), type(type_), platform_id(platform_id_), headline(headline_), page_content(page_content_) {}

    topgg::platform platform{};
    project_type type{};
    std::string platform_id{};
    std::string headline{};
    std::string page_content{};
  };
#endif
};