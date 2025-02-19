/**
 * @module topgg
 * @file models.h
 * @brief The official C++ wrapper for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024 Top.gg & null8626
 * @date 2025-02-19
 * @version 3.0.0
 */

#pragma once

#include <topgg/topgg.h>

#include <string>
#include <optional>
#include <string>
#include <vector>

#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE
#endif

#include <ctime>

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#define TOPGG_BOT_QUERY_SORT(lib_name, api_name)    \
  inline bot_query& sort_by_##lib_name() noexcept { \
    m_sort = #api_name;                             \
    return *this;                                   \
  }

#define TOPGG_BOT_QUERY_QUERY(type, name, ...)   \
  inline bot_query& name(const type name) {      \
    add_query(#name, name, __VA_ARGS__);         \
    return *this;                                \
  }

#define TOPGG_BOT_QUERY_SEARCH(type, name, ...)  \
  inline bot_query& name(const type name) {      \
    add_search(#name, name, __VA_ARGS__);        \
    return *this;                                \
  }

namespace topgg {
  /**
   * @brief Base class of the account data stored in the Top.gg API.
   *
   * @see topgg::bot
   * @see topgg::user
   * @see topgg::voter
   * @since 2.0.0
   */
  class TOPGG_EXPORT account {
  protected:
    account(const dpp::json& j);

  public:
    account() = delete;

    /**
     * @brief The account's Discord ID.
     *
     * @since 2.0.0
     */
    dpp::snowflake id;

    /**
     * @brief The account's entire Discord avatar URL.
     *
     * @note This avatar URL can be animated if possible.
     * @since 2.0.0
     */
    std::string avatar;

    /**
     * @brief The account's username.
     *
     * @since 2.0.0
     */
    std::string username;

    /**
     * @brief The unix timestamp of when this account was created.
     *
     * @since 2.0.0
     */
    time_t created_at;
  };

  class bot_query;
  class client;

  /**
   * @brief Represents voters of a Discord bot.
   *
   * @see topgg::client::get_voters
   * @see topgg::client::start_autoposter
   * @see topgg::account
   * @since 2.0.0
   */
  class TOPGG_EXPORT voter: public account {
    inline voter(const dpp::json& j)
      : account(j) {}

  public:
    voter() = delete;

    friend class client;
  };

  /**
   * @brief Represents a Discord bot listed on Top.gg.
   *
   * @see topgg::client::get_bot
   * @see topgg::account
   * @since 2.0.0
   */
  class TOPGG_EXPORT bot: public account {
    bot(const dpp::json& j);

  public:
    bot() = delete;

    /**
     * @brief The Discord bot's command prefix.
     *
     * @since 2.0.0
     */
    std::string prefix;

    /**
     * @brief The Discord bot's short description.
     *
     * @since 2.0.0
     */
    std::string short_description;

    /**
     * @brief The Discord bot's long description, if available.
     *
     * @note This long description can contain Markdown and/or HTML.
     * @since 2.0.0
     */
    std::optional<std::string> long_description;

    /**
     * @brief A list of the Discord bot's tags.
     *
     * @since 2.0.0
     */
    std::vector<std::string> tags;

    /**
     * @brief A link to the Discord bot's website, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> website;

    /**
     * @brief A link to the Discord bot's GitHub repository, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> github;

    /**
     * @brief A list of the Discord bot's owners, represented in Discord user IDs.
     *
     * @since 2.0.0
     */
    std::vector<dpp::snowflake> owners;

    /**
     * @brief The Discord bot's page banner URL, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> banner;

    /**
     * @brief The unix timestamp of when this Discord bot was approved on Top.gg by a Bot Reviewer.
     *
     * @since 2.0.0
     */
    time_t approved_at;

    /**
     * @brief The amount of upvotes this Discord bot has.
     *
     * @since 2.0.0
     */
    size_t votes;

    /**
     * @brief The amount of upvotes this Discord bot has this month.
     *
     * @since 2.0.0
     */
    size_t monthly_votes;

    /**
     * @brief The Discord bot's support server invite URL, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> support;

    /**
     * @brief The invite URL of this Discord bot.
     *
     * @since 2.0.0
     */
    std::string invite;

    /**
     * @brief The URL of this Discord bot’s Top.gg page.
     *
     * @since 2.0.0
     */
    std::string url;

    friend class bot_query;
    friend class client;
  };

  /**
   * @brief The callback function to call when get_bots completes.
   *
   * @see topgg::client::get_bots
   * @see topgg::bot_query
   * @since 3.0.0
   */
  using get_bots_completion_t = std::function<void(const result<std::vector<bot>>&)>;

  class TOPGG_EXPORT bot_query {
    client* m_client;
    std::string m_query;
    std::string m_search;
    const char* m_sort;
  
    inline bot_query(client* c): m_client(c), m_query("/bots?"), m_sort(nullptr) {}
    
    void add_query(const char* key, const uint16_t value, const uint16_t max);
    void add_query(const char* key, const char* value);
    void add_search(const char* key, const std::string& value);
    void add_search(const char* key, const size_t value);

  public:
    bot_query() = delete;

    TOPGG_BOT_QUERY_SORT(approval_date, date);
    TOPGG_BOT_QUERY_SORT(monthly_votes, monthlyPoints);
    TOPGG_BOT_QUERY_QUERY(uint16_t, limit, 500);
    TOPGG_BOT_QUERY_QUERY(uint16_t, skip, 499);
    TOPGG_BOT_QUERY_SEARCH(std::string&, username);
    TOPGG_BOT_QUERY_SEARCH(std::string&, prefix);
    TOPGG_BOT_QUERY_SEARCH(size_t, votes);
    TOPGG_BOT_QUERY_SEARCH(size_t, monthly_votes);
    TOPGG_BOT_QUERY_SEARCH(std::string&, vanity);
    
    void finish(const get_bots_completion_t& callback);

    friend class client;
  };

  class autoposter_source {
  public:
    virtual size_t get_server_count(dpp::cluster&) = 0;
  };

  class user;

  /**
   * @brief Represents a user's social links, if available.
   *
   * @see topgg::user
   * @since 2.0.0
   */
  class TOPGG_EXPORT user_socials {
    user_socials(const dpp::json& j);

  public:
    user_socials() = delete;

    /**
     * @brief A URL of this user’s GitHub account, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> github;

    /**
     * @brief A URL of this user’s Instagram account, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> instagram;

    /**
     * @brief A URL of this user’s Reddit account, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> reddit;

    /**
     * @brief A URL of this user’s Twitter/X account, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> twitter;

    /**
     * @brief A URL of this user’s YouTube channel, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> youtube;

    friend class user;
  };

  /**
   * @brief Represents a user logged into Top.gg.
   *
   * @see topgg::user_socials
   * @see topgg::client::get_user
   * @see topgg::voter
   * @see topgg::account
   * @since 2.0.0
   */
  class TOPGG_EXPORT user: public account {
    user(const dpp::json& j);

  public:
    user() = delete;

    /**
     * @brief The user's bio, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> bio;

    /**
     * @brief The URL of this user’s profile banner image, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> banner;

    /**
     * @brief This user's social links, if available.
     *
     * @since 2.0.0
     */
    std::optional<user_socials> socials;

    /**
     * @brief Whether this user is a Top.gg supporter or not.
     *
     * @since 2.0.0
     */
    bool is_supporter;

    /**
     * @brief Whether this user is a Top.gg moderator or not.
     *
     * @since 2.0.0
     */
    bool is_moderator;

    /**
     * @brief Whether this user is a Top.gg website moderator or not.
     *
     * @since 2.0.0
     */
    bool is_web_moderator;

    /**
     * @brief Whether this user is a Top.gg website administrator or not.
     *
     * @since 2.0.0
     */
    bool is_admin;

    friend class client;
  };
}; // namespace topgg

#undef TOPGG_BOT_QUERY_SEARCH
#undef TOPGG_BOT_QUERY_QUERY
#undef TOPGG_BOT_QUERY_SORT