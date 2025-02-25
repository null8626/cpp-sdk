/**
 * @module topgg
 * @file models.h
 * @brief The official C++ wrapper for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-02-25
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
  class bot_query;
  class client;

  /**
   * @brief Represents voters of a Discord bot.
   *
   * @see topgg::client::get_voters
   * @see topgg::client::start_autoposter
   * @since 2.0.0
   */
  class TOPGG_EXPORT voter {
  public:
    voter() = delete;

    /**
     * @brief This voter's Discord ID.
     *
     * @since 2.0.0
     */
    dpp::snowflake id;

    /**
     * @brief This voter's avatar URL.
     *
     * @since 2.0.0
     */
    std::string avatar;

    /**
     * @brief This voter's username.
     *
     * @since 3.0.0
     */
    std::string name;

    /**
     * @brief This voter's creation date.
     *
     * @since 2.0.0
     */
    time_t created_at;

    voter(const dpp::json& j);

    friend class client;
  };

  /**
   * @brief Represents a Discord bot listed on Top.gg.
   *
   * @see topgg::client::get_bot
   * @since 2.0.0
   */
  class TOPGG_EXPORT bot {
    bot(const dpp::json& j);

  public:
    bot() = delete;

    /**
     * @brief This bot's Discord ID.
     *
     * @since 2.0.0
     */
    dpp::snowflake id;

    /**
     * @brief This bot's Top.gg ID.
     *
     * @since 3.0.0
     */
    dpp::snowflake topgg_id;

    /**
     * @brief This bot's avatar URL.
     *
     * @since 2.0.0
     */
    std::string avatar;

    /**
     * @brief This bot's username.
     *
     * @since 3.0.0
     */
    std::string name;

    /**
     * @brief This bot's creation date.
     *
     * @since 2.0.0
     */
    time_t created_at;

    /**
     * @brief This bot's prefix.
     *
     * @since 2.0.0
     */
    std::string prefix;

    /**
     * @brief This bot's short description.
     *
     * @since 2.0.0
     */
    std::string short_description;

    /**
     * @brief This bot's long description.
     *
     * @note This long description can contain Markdown and/or HTML.
     * @since 2.0.0
     */
    std::optional<std::string> long_description;

    /**
     * @brief This bot's tags.
     *
     * @since 2.0.0
     */
    std::vector<std::string> tags;

    /**
     * @brief This bot's website URL.
     *
     * @since 2.0.0
     */
    std::optional<std::string> website;

    /**
     * @brief This bot's GitHub repository URL.
     *
     * @since 2.0.0
     */
    std::optional<std::string> github;

    /**
     * @brief This bot's owners IDs.
     *
     * @since 2.0.0
     */
    std::vector<dpp::snowflake> owners;

    /**
     * @brief This bot's banner URL.
     *
     * @since 2.0.0
     */
    std::optional<std::string> banner;

    /**
     * @brief This bot's submission date.
     *
     * @since 3.0.0
     */
    time_t submitted_at;

    /**
     * @brief The amount of votes this bot has.
     *
     * @since 2.0.0
     */
    size_t votes;

    /**
     * @brief The amount of votes this bot has this month.
     *
     * @since 2.0.0
     */
    size_t monthly_votes;

    /**
     * @brief This bot's support URL.
     *
     * @since 2.0.0
     */
    std::optional<std::string> support;

    /**
     * @brief This bot's invite URL.
     *
     * @since 2.0.0
     */
    std::optional<std::string> invite;

    /**
     * @brief This bot's Top.gg page URL.
     *
     * @since 2.0.0
     */
    std::string url;

    /**
     * @brief This bot's posted server count.
     *
     * @since 3.0.0
     */
    std::optional<size_t> server_count;

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
   * @brief A class for configuring the query in get_bots before being sent to the API.
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
     * @brief Sorts results based on each bot's submission date.
     * 
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 3.0.0
     */
    TOPGG_BOT_QUERY_SORT(submission_date, date);

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
    TOPGG_BOT_QUERY_QUERY(uint16_t, offset, 499);

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
     * @brief Sends the query to the API.
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
     *   .offset(50)
     *   .username("shiro")
     *   .sort_by_monthly_votes()
     *   .send([](const auto& result) {
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
     * @param callback The callback function to call when send( completes.
     * @note For its C++20 coroutine counterpart, see co_send(.
     * @see topgg::client::get_bots
     * @see topgg::bot_query::co_send(
     * @since 2.0.1
     */
    void send(const get_bots_completion_t& callback);

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
     *     .offset(50)
     *     .username("shiro")
     *     .sort_by_monthly_votes()
     *     .send();
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
     * @see topgg::bot_query::co_send(
     * @since 2.0.1
     */
    topgg::async_result<std::vector<topgg::bot>> co_send();
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
