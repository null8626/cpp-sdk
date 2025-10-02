/**
 * @module topgg
 * @file models.h
 * @brief A community-maintained C++ API Client for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-10-02
 * @version 2.1.0
 */

#pragma once

#include <topgg/topgg.h>

#include <unordered_map>
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

#ifdef __TOPGG_BUILDING__
#define _TOPGG_SNOWFLAKE_FROM_JSON(j, name) \
  dpp::snowflake{j[#name].template get<std::string>()}
#endif

#define TOPGG_BOT_QUERY_SORT(lib_name, api_name)               \
  inline bot_query& sort_by_##lib_name() noexcept {            \
    m_sort = #api_name;                                        \
    return *this;                                              \
  }

#define TOPGG_BOT_QUERY_QUERY(type, lib_name, api_name, ...)   \
  inline bot_query& lib_name(const type lib_name) {            \
    add_query(#api_name, lib_name, __VA_ARGS__);               \
    return *this;                                              \
  }

namespace topgg {
  class bot_query;
  class client;

  /**
   * @brief A Top.gg account.
   *
   * @see topgg::bot
   * @see topgg::user
   * @see topgg::voter
   * @since 2.0.0
   */
  class TOPGG_EXPORT account {
  protected:
    account(const dpp::json& j, const char* id_key = "id");

  public:
    account() = delete;

    /**
     * @brief This account's ID.
     *
     * @since 2.0.0
     */
    dpp::snowflake id;

    /**
     * @brief This account's avatar URL.
     *
     * @since 2.0.0
     */
    std::string avatar;

    /**
     * @brief This account's username.
     *
     * @since 2.0.0
     */
    std::string username;

    /**
     * @brief This account's creation date.
     *
     * @since 2.0.0
     */
    time_t created_at;

    friend class client;
  };

  class client;

  /**
   * @brief A Top.gg voter.
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
   * @brief A Discord bot listed on Top.gg.
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
     * @brief This bot's Top.gg ID.
     *
     * @since 2.1.0
     */
    dpp::snowflake topgg_id;

    /**
     * @brief The Discord bot's discriminator.
     *
     * @since 2.0.0
     */
    std::string discriminator;

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
     * @brief This bot's owner IDs.
     *
     * @since 2.0.0
     */
    std::vector<dpp::snowflake> owners;

    /**
     * @brief A list of IDs of the guilds featured on this Discord bot’s page.
     *
     * @since 2.0.0
     */
    std::vector<size_t> guilds;

    /**
     * @brief The Discord bot's page banner URL, if available.
     *
     * @since 2.0.0
     */
    std::optional<std::string> banner;

    /**
     * @brief This bot's submission date.
     *
     * @since 2.1.0
     */
    time_t submitted_at;

    /**
     * @brief Whether this Discord bot is Top.gg certified or not.
     *
     * @since 2.0.0
     */
    bool is_certified;

    /**
     * @brief A list of this Discord bot’s shards.
     *
     * @since 2.0.0
     */
    std::vector<size_t> shards;

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
     * @brief The amount of shards this Discord bot has according to posted stats.
     *
     * @since 2.0.0
     */
    size_t shard_count;

    /**
     * @brief This bot's invite URL.
     *
     * @since 2.0.0
     */
    std::optional<std::string> invite;

    /**
     * @brief The URL of this Discord bot’s Top.gg page.
     *
     * @since 2.0.0
     */
    std::string url;

    /**
     * @brief This bot's Top.gg vanity code.
     *
     * @since 2.1.0
     */
    std::optional<std::string> vanity;

    /**
     * @brief This bot's posted server count.
     *
     * @since 2.1.0
     */
    std::optional<size_t> server_count;

    /**
     * @brief This bot's average review score out of 5.
     *
     * @since 2.1.0
     */
    double review_score;

    /**
     * @brief This bot's review count.
     *
     * @since 2.1.0
     */
    size_t review_count;

    friend class bot_query;
    friend class client;
  };

  /**
   * @brief Represents a Discord bot’s statistics.
   *
   * @see topgg::voter
   * @see topgg::client::get_stats
   * @see topgg::client::post_stats
   * @see topgg::client::start_autoposter
   * @since 2.0.0
   */
  class TOPGG_EXPORT stats {
    stats(const dpp::json& j);

    std::optional<size_t> m_shard_count;
    std::optional<std::vector<size_t>> m_shards;
    std::optional<size_t> m_shard_id;
    std::optional<size_t> m_server_count;

    std::string to_json() const;

  public:
    stats() = delete;

    /**
     * @brief Creates a stats object based on existing data from your D++ cluster instance.
     *
     * @param bot The D++ cluster instance.
     * @since 2.0.0
     */
    stats(dpp::cluster& bot);

    /**
     * @brief Creates a stats object based on the bot's server and shard count.
     *
     * @param server_count The amount of servers this Discord bot has.
     * @param shard_count The amount of shards this Discord bot has. Defaults to one.
     * @since 2.0.0
     */
    inline stats(const size_t server_count, const size_t shard_count = 1)
      : m_shard_count(std::optional{shard_count}), m_server_count(std::optional{server_count}) {}

    /**
     * @brief Creates a stats object based on the bot's shard data.
     *
     * @param shards An array of this bot's server count for each shard.
     * @param shard_index The array index of the shard posting this data, defaults to zero.
     * @throw std::out_of_range If the shard_index argument is out of bounds from the shards argument.
     * @since 2.0.0
     */
    stats(const std::vector<size_t>& shards, const size_t shard_index = 0);

    /**
     * @brief Returns this stats object's server count for each shard.
     * @return std::vector<size_t> This stats object's server count for each shard.
     * @since 2.0.0
     */
    std::vector<size_t> shards() const noexcept;

    /**
     * @brief Returns this stats object's shard count.
     * @return size_t This stats object's shard count.
     * @since 2.0.0
     */
    size_t shard_count() const noexcept;

    /**
     * @brief Returns this stats object's server count, if available.
     * @return std::optional<size_t> This stats object's server count, if available.
     * @since 2.0.0
     */
    std::optional<size_t> server_count() const noexcept;

    /**
     * @brief Sets this stats object's server count.
     *
     * @param new_server_count The new server count.
     * @since 2.0.0
     */
    inline void set_server_count(const size_t new_server_count) noexcept {
      m_server_count = std::optional{new_server_count};
    }

    friend class client;
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
     * @brief Whether this user is a Top.gg certified developer or not.
     *
     * @since 2.0.0
     */
    bool is_certified_dev;

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

  /**
   * @brief The callback function to call when get_bots completes.
   *
   * @see topgg::client::get_bots
   * @see topgg::bot_query
   * @since 2.0.1
   */
  using get_bots_completion_t = std::function<void(const result<std::vector<bot>>&)>;

  /**
   * @brief Configure a Discord bot query before sending it to the API.
   *
   * @see topgg::client::get_bots
   * @since 2.0.1
   */
  class TOPGG_EXPORT bot_query {
    client* m_client;
    std::unordered_map<const char*, std::string> m_query;
    const char* m_sort;

    inline bot_query(client* c)
      : m_client(c), m_sort(nullptr) {}

    void add_query(const char* key, const uint16_t value, const uint16_t max);

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
     * @since 2.1.0
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
    TOPGG_BOT_QUERY_QUERY(uint16_t, limit, limit, 500);

    /**
     * @brief Sets the amount of bots to be skipped.
     *
     * @param skip The amount of bots to be skipped. This cannot be more than 499.
     * @return bot_query The current modified object.
     * @see topgg::client::get_bots
     * @since 2.0.1
     */
    TOPGG_BOT_QUERY_QUERY(uint16_t, skip, offset, 499);

    /**
     * @brief Sends the query to the API.
     *
     * @param callback The callback function to call when send() completes.
     * @note For its C++20 coroutine counterpart, see co_send().
     * @see topgg::client::get_bots
     * @see topgg::bot_query::co_send
     * @since 2.0.1
     */
    void send(const get_bots_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Sends the query to the API through a C++20 coroutine.
     *
     * @throw topgg::internal_server_error Unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Invalid API token.
     * @throw topgg::ratelimited Ratelimited from sending more requests.
     * @throw dpp::http_error An unexpected HTTP exception has occured.
     * @return co_await to retrieve a vector of topgg::bot if successful
     * @note For its C++17 callback-based counterpart, see get_bot.
     * @see topgg::client::get_bots
     * @see topgg::bot_query::send
     * @since 2.0.1
     */
    topgg::async_result<std::vector<topgg::bot>> co_send();
#endif

    friend class client;
  };

  /**
   * @brief An abstract interface for bots that have a custom way of retrieving their statistics.
   *
   * @see topgg::start_autoposter
   * @since 2.1.0
   */
  class autoposter_source {
  public:
    virtual stats TOPGG_EXPORT get_stats(dpp::cluster&) = 0;
  };
}; // namespace topgg

#undef TOPGG_BOT_QUERY_SEARCH
#undef TOPGG_BOT_QUERY_QUERY
#undef TOPGG_BOT_QUERY_SORT