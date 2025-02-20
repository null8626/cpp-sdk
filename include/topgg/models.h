/**
 * @module topgg
 * @file models.h
 * @brief The official C++ wrapper for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-02-20
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
     * @brief The URL of this Discord bot's Top.gg page.
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
   * @since 2.0.1
   */
  using get_bots_completion_t = std::function<void(const result<std::vector<bot>>&)>;

  /**
   * @brief A class for configuring the query in get_bots before being sent to the Top.gg API.
   *
   * @see topgg::client::get_bots
   * @since 2.0.1
   */
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

    /**
     * @brief Sorts results based on each bot's ID.
     * 
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SORT(id, id);

    /**
     * @brief Sorts results based on each bot's approval date.
     * 
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SORT(approval_date, date);

    /**
     * @brief Sorts results based on each bot's monthly vote count.
     * 
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SORT(monthly_votes, monthlyPoints);

    /**
     * @brief Sets the maximum amount of bots to be queried.
     * 
     * @param limit The maximum amount of bots to be queried. This cannot be more than 500.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_QUERY(uint16_t, limit, 500);

    /**
     * @brief Sets the amount of bots to be skipped during the query.
     * 
     * @param skip The amount of bots to be skipped during the query. This cannot be more than 499.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_QUERY(uint16_t, skip, 499);

    /**
     * @brief Queries only Discord bots that has this username.
     * 
     * @param username The bot's username.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SEARCH(std::string&, username);

    /**
     * @brief Queries only Discord bots that has this prefix.
     * 
     * @param prefix The bot's prefix.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SEARCH(std::string&, prefix);

    /**
     * @brief Queries only Discord bots that has this vote count.
     * 
     * @param votes The bot's vote count.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SEARCH(size_t, votes);

    /**
     * @brief Queries only Discord bots that has this monthly vote count.
     * 
     * @param monthly_votes The bot's monthly vote count.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SEARCH(size_t, monthly_votes);

    /**
     * @brief Queries only Discord bots that has this vanity URL.
     * 
     * @param vanity The bot's vanity URL.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_SEARCH(std::string&, vanity);
    
    /**
     * @brief Sends the query to the Top.gg API.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client
     *   .get_bots()
     *   .limit(250)
     *   .skip(50)
     *   .username("shiro")
     *   .sort_by_monthly_votes()
     *   .finish([](const auto& result) {
     *     try {
     *       const auto bots = result.get();
     *   
     *       for (const auto& bot: bots) {
     *         std::cout << bot.username << std::endl;
     *       }
     *     } catch (const std::exception& exc) {
     *       std::cerr << "error: " << exc.what() << std::endl;
     *     }
     *   });
     * ```
     * 
     * @param callback The callback function to call when finish completes.
     * @note For its C++20 coroutine counterpart, see co_finish.
     * @see topgg::client::get_bots
     * @see topgg::bot_query::co_finish
     * @since 2.0.1
     */
    void finish(const get_bots_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Sends the query to the Top.gg API through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * try {
     *   const auto bots = co_await topgg_client
     *     .get_bots()
     *     .limit(250)
     *     .skip(50)
     *     .username("shiro")
     *     .sort_by_monthly_votes()
     *     .finish();
     *
     *   for (const auto& bot: bots) {
     *     std::cout << topgg_bot.username << std::endl;
     *   }
     * } catch (const std::exception& exc) {
     *   std::cerr << "error: " << exc.what() << std::endl;
     * }
     * ```
     * 
     * @throw topgg::internal_server_error Thrown when the client receives an unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Thrown when its known that the client uses an invalid Top.gg API token.
     * @throw topgg::not_found Thrown when such query does not exist.
     * @throw topgg::ratelimited Thrown when the client gets ratelimited from sending more HTTP requests.
     * @throw dpp::http_error Thrown when an unexpected HTTP exception has occured.
     * @return co_await to retrieve a vector of topgg::bot if successful
     * @note For its C++17 callback-based counterpart, see get_bot.
     * @see topgg::client::get_bots
     * @see topgg::bot_query::co_finish
     * @since 2.0.1
     */
    topgg::async_result<std::vector<topgg::bot>> co_finish();
#endif

    friend class client;
  };

  /**
   * @brief An abstract interface for bots that have a custom way of retrieving their server count.
   *
   * @see topgg::start_autoposter
   * @since 3.0.0
   */
  class autoposter_source {
  public:
    virtual size_t TOPGG_EXPORT get_server_count(dpp::cluster&) = 0;
  };
}; // namespace topgg

#undef TOPGG_BOT_QUERY_SEARCH
#undef TOPGG_BOT_QUERY_QUERY
#undef TOPGG_BOT_QUERY_SORT