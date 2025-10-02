/**
 * @module topgg
 * @file client.h
 * @brief A community-maintained C++ API Client for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-10-02
 * @version 3.0.0
 */

#pragma once

#include <topgg/topgg.h>

#include <functional>
#include <vector>
#include <string>
#include <map>

namespace topgg {
  /**
   * @brief The callback function to call when get_bot completes.
   *
   * @see topgg::client::get_bot
   * @since 2.0.0
   */
  using get_bot_completion_t = std::function<void(const result<bot>&)>;

  /**
   * @brief The callback function to call when get_user completes.
   *
   * @see topgg::client::get_user
   * @since 2.0.0
   */
  using get_user_completion_t = std::function<void(const result<user>&)>;

  /**
   * @brief The callback function to call when get_stats completes.
   *
   * @see topgg::client::get_stats
   * @since 2.0.0
   */
  using get_stats_completion_t = std::function<void(const result<stats>&)>;

  /**
   * @brief The callback function to call when get_voters completes.
   *
   * @see topgg::client::get_voters
   * @since 2.0.0
   */
  using get_voters_completion_t = std::function<void(const result<std::vector<voter>>&)>;

  /**
   * @brief The callback function to call when has_voted completes.
   *
   * @see topgg::client::has_voted
   * @since 2.0.0
   */
  using has_voted_completion_t = std::function<void(const result<bool>&)>;

  /**
   * @brief The callback function to call when is_weekend completes.
   *
   * @see topgg::client::is_weekend
   * @since 2.0.0
   */
  using is_weekend_completion_t = std::function<void(const result<bool>&)>;

  /**
   * @brief The callback function to call when post_stats completes.
   *
   * @see topgg::client::post_stats
   * @since 2.0.0
   */
  using post_stats_completion_t = std::function<void(const bool)>;

  /**
   * @brief The callback function to call after every autopost request to the API, successful or not.
   *
   * @see topgg::client::start_autoposter
   * @see topgg::client::stop_autoposter
   * @since 2.0.0
   */
  using custom_autopost_callback_t = std::function<::topgg::stats(dpp::cluster&)>;

  /**
   * @brief Interact with the API's endpoints.
   *
   * @since 2.0.0
   */
  class TOPGG_EXPORT client {
    dpp::http_headers m_headers;
    dpp::snowflake m_id;
    dpp::cluster& m_cluster;
    dpp::timer m_autoposter_timer;

    void request(const dpp::http_method method, const std::string& url, const dpp::http_completion_event callback, const std::string& body = "") {
      m_cluster.request(TOPGG_BASE_URL + url, method, callback, body, "application/json", m_headers);
    }
    
    template<typename T>
    void basic_request(const dpp::http_method method, const std::string& url, const std::function<void(const result<T>&)>& callback, std::function<T(const dpp::json&)>&& conversion_fn, const std::string& body = "") {
      request(method, url, [callback, conversion_fn_in = std::move(conversion_fn)](const dpp::http_request_completion_t& response) { callback(result<T>{response, conversion_fn_in}); }, body);
    }

    stats get_stats();
    void post_stats_inner(const size_t server_count, const dpp::http_completion_event callback);

  public:
    client() = delete;

    /**
     * @brief Creates a client object.
     *
     * @param cluster A pointer to the bot's D++ cluster using this library.
     * @param token The API token to use.
     * @since 2.0.0
     */
    client(dpp::cluster& cluster, const std::string& token);

    /**
     * @brief This object can't be copied.
     *
     * @param other Other object to copy from.
     * @since 2.0.0
     */
    client(const client& other) = delete;

    /**
     * @brief This object can't be moved.
     *
     * @param other Other object to move from.
     * @since 2.0.0
     */
    client(client&& other) = delete;

    /**
     * @brief This object can't be copied.
     *
     * @param other Other object to copy from.
     * @return client The current modified object.
     * @since 2.0.0
     */
    client& operator=(const client& other) = delete;

    /**
     * @brief This object can't be moved.
     *
     * @param other Other object to move from.
     * @return client The current modified object.
     * @since 2.0.0
     */
    client& operator=(client&& other) = delete;

    /**
     * @brief Fetches a Discord bot from its ID.
     *
     * @param bot_id The requested ID.
     * @param callback The callback function to call when get_bot completes.
     * @note For its C++20 coroutine counterpart, see co_get_bot.
     * @see topgg::result
     * @see topgg::bot
     * @see topgg::client::co_get_bot
     * @since 2.0.0
     */
    void get_bot(const dpp::snowflake bot_id, const get_bot_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Fetches a Discord bot from its ID through a C++20 coroutine.
     *
     * @param bot_id The requested ID.
     * @throw topgg::internal_server_error Unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Invalid API token.
     * @throw topgg::not_found The specified bot does not exist.
     * @throw topgg::ratelimited Ratelimited from sending more requests.
     * @throw dpp::http_error An unexpected HTTP exception has occured.
     * @return co_await to retrieve a topgg::bot if successful
     * @note For its C++17 callback-based counterpart, see get_bot.
     * @see topgg::async_result
     * @see topgg::bot
     * @see topgg::client::get_bot
     * @since 2.0.0
     */
    topgg::async_result<topgg::bot> co_get_bot(const dpp::snowflake bot_id);
#endif

    /**
     * @deprecated No longer supported by API v0.
     * 
     * @brief Fetches a user from a Discord ID.
     *
     * @param user_id The Discord user ID to fetch from.
     * @param callback The callback function to call when get_user completes.
     * @note For its C++20 coroutine counterpart, see co_get_user.
     * @see topgg::result
     * @see topgg::user
     * @see topgg::co_get_user
     * @since 2.0.0
     */
    void get_user(const dpp::snowflake user_id, const get_user_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @deprecated No longer supported by API v0.
     * 
     * @brief Fetches a user from a Discord ID through a C++20 coroutine.
     *
     * @param user_id The Discord user ID to fetch from.
     * @throw topgg::internal_server_error Thrown when the client receives an unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Thrown when its known that the client uses an invalid Top.gg API token.
     * @throw topgg::not_found Thrown when such query does not exist.
     * @throw topgg::ratelimited Thrown when the client gets ratelimited from sending more HTTP requests.
     * @throw dpp::http_error Thrown when an unexpected HTTP exception occured.
     * @return co_await to retrieve a topgg::user if successful
     * @note For its C++17 callback-based counterpart, see get_user.
     * @see topgg::async_result
     * @see topgg::bot
     * @see topgg::client::get_user
     * @since 2.0.0
     */
    topgg::async_result<topgg::user> co_get_user(const dpp::snowflake user_id);
#endif

    /**
     * @brief Returns an object that allows you to configure a bot query before sending it to the API.
     *
     * @return bot_query An object that allows you to configure a bot query before sending it to the API.
     * @see topgg::bot_query
     * @since 2.0.1
     */
    inline bot_query get_bots() noexcept {
      return bot_query{this};
    }

    /**
     * @brief Fetches your Discord bot's posted statistics.
     *
     * @param callback The callback function to call when get_stats completes.
     * @note For its C++20 coroutine counterpart, see co_get_stats.
     * @see topgg::result
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_get_stats
     * @since 2.0.0
     */
    void get_stats(const get_stats_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Fetches your Discord bot's posted statistics through a C++20 coroutine.
     *
     * @throw topgg::internal_server_error Unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Invalid API token.
     * @throw topgg::ratelimited Ratelimited from sending more requests.
     * @throw dpp::http_error An unexpected HTTP exception has occured.
     * @return co_await to retrieve an optional size_t if successful
     * @note For its C++17 callback-based counterpart, see get_stats.
     * @see topgg::async_result
     * @see topgg::client::start_autoposter
     * @see topgg::client::get_stats
     * @since 2.0.0
     */
    topgg::async_result<topgg::stats> co_get_stats();
#endif

    /**
     * @brief Fetches your project's recent 100 unique voters.
     *
     * @param page The page number. Each page can only have at most 100 voters.
     * @param callback The callback function to call when get_voters completes.
     * @note For its C++20 coroutine counterpart, see co_get_voters.
     * @see topgg::result
     * @see topgg::voter
     * @see topgg::stats
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_get_voters
     * @since 2.0.0
     */
    void get_voters(size_t page, const get_voters_completion_t& callback);

    /**
     * @brief Fetches your project's recent 100 unique voters.
     *
     * @param callback The callback function to call when get_voters completes.
     * @note For its C++20 coroutine counterpart, see co_get_voters.
     * @see topgg::result
     * @see topgg::voter
     * @see topgg::stats
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_get_voters
     * @since 2.0.0
     */
    void get_voters(const get_voters_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Fetches your project's recent 100 unique voters through a C++20 coroutine.
     *
     * @param page The page number. Each page can only have at most 100 voters. Defaults to 1.
     * @throw topgg::internal_server_error Unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Invalid API token.
     * @throw topgg::ratelimited Ratelimited from sending more requests.
     * @throw dpp::http_error An unexpected HTTP exception has occured.
     * @return co_await to retrieve a vector of topgg::voter if successful
     * @note For its C++17 callback-based counterpart, see get_voters.
     * @see topgg::async_result
     * @see topgg::voter
     * @see topgg::stats
     * @see topgg::client::start_autoposter
     * @see topgg::client::get_voters
     * @since 2.0.0
     */
    topgg::async_result<std::vector<voter>> co_get_voters(size_t page = 1);
#endif

    /**
     * @brief Checks if the specified Top.gg user has voted for your project in the past 12 hours.
     *
     * @param user_id The requested user's ID.
     * @param callback The callback function to call when has_voted completes.
     * @note For its C++20 coroutine counterpart, see co_has_voted.
     * @see topgg::result
     * @see topgg::stats
     * @see topgg::client::start_autoposter
     * @since 2.0.0
     */
    void has_voted(const dpp::snowflake user_id, const has_voted_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Checks if the specified Top.gg user has voted for your project in the past 12 hours through a C++20 coroutine.
     *
     * @param user_id The requested user's ID.
     * @throw topgg::internal_server_error Unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Invalid API token.
     * @throw topgg::not_found The specified user has not logged in to Top.gg.
     * @throw topgg::ratelimited Ratelimited from sending more requests.
     * @throw dpp::http_error An unexpected HTTP exception has occured.
     * @return co_await to retrieve a bool if successful
     * @note For its C++17 callback-based counterpart, see has_voted.
     * @see topgg::async_result
     * @see topgg::stats
     * @see topgg::client::start_autoposter
     * @see topgg::client::has_voted
     * @since 2.0.0
     */
    topgg::async_result<bool> co_has_voted(const dpp::snowflake user_id);
#endif

    /**
     * @brief Checks if the weekend multiplier is active, where a single vote counts as two.
     *
     * @param callback The callback function to call when is_weekend completes.
     * @note For its C++20 coroutine counterpart, see co_is_weekend.
     * @see topgg::result
     * @see topgg::client::co_is_weekend
     * @since 2.0.0
     */
    void is_weekend(const is_weekend_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Checks if the weekend multiplier is active through a C++20 coroutine, where a single vote counts as two.
     *
     * @throw topgg::internal_server_error Unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Invalid API token.
     * @throw topgg::ratelimited Ratelimited from sending more requests.
     * @throw dpp::http_error An unexpected HTTP exception has occured.
     * @return co_await to retrieve a bool if successful
     * @note For its C++17 callback-based counterpart, see is_weekend.
     * @see topgg::async_result
     * @see topgg::client::is_weekend
     * @since 2.0.0
     */
    topgg::async_result<bool> co_is_weekend();
#endif

    /**
     * @brief Posts your Discord bot's statistics to the API. This will update the statistics in your bot's Top.gg page.
     *
     * @param callback The callback function to call when post_stats completes.
     * @note For its C++20 coroutine counterpart, see co_post_stats.
     * @see topgg::result
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_post_stats
     * @since 2.0.0
     */
    void post_stats(const post_stats_completion_t& callback);

#ifdef DPP_CORO
    /**
     * @brief Posts your Discord bot's statistics to the API through a C++20 coroutine. This will update the statistics in your bot's Top.gg page.
     *
     * @return co_await to retrieve a bool
     * @note For its C++17 callback-based counterpart, see post_stats.
     * @see topgg::client::start_autoposter
     * @see topgg::client::post_stats
     * @since 2.0.0
     */
    dpp::async<bool> co_post_stats();
#endif

    /**
     * @brief Starts autoposting your Discord bot's statistics using data directly from your D++ cluster instance.
     *
     * @param callback The callback function to call after every request to the API, successful or not.
     * @param interval The interval between posting in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_stats
     * @see topgg::client::stop_autoposter
     * @see topgg::custom_autopost_callback_t
     * @since 2.0.0
     */
    void start_autoposter(const post_stats_completion_t& callback, time_t interval = TOPGG_AUTOPOSTER_MIN_INTERVAL);

    /**
     * @brief Starts autoposting your Discord bot's statistics using data directly from your D++ cluster instance.
     *
     * @param stats_callback The callback function to call after every request to the API, successful or not.
     * @param post_callback The callback function that retrieves your bot's statistics.
     * @param interval The interval between posting in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_stats
     * @see topgg::client::stop_autoposter
     * @see topgg::custom_autopost_callback_t
     * @since 3.0.0
     */
    void start_autoposter(const custom_autopost_callback_t& stats_callback, const post_stats_completion_t& post_callback, time_t interval);

    /**
     * @brief Starts autoposting your Discord bot's statistics using data directly from your D++ cluster instance.
     *
     * @param interval The interval between posting in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_stats
     * @see topgg::client::stop_autoposter
     * @since 2.0.0
     */
    void start_autoposter(time_t interval = TOPGG_AUTOPOSTER_MIN_INTERVAL);

    /**
     * @brief Starts autoposting your Discord bot's statistics using a custom data source.
     *
     * @param source A pointer to a autoposter statistics source located in the heap memory. This pointer must be allocated with new, and it will be deleted once the autoposter thread gets stopped.
     * @param callback The callback function to call after every request to the API, successful or not.
     * @param interval The interval between posting in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_stats
     * @see topgg::client::stop_autoposter
     * @see topgg::post_stats_completion_t
     * @see topgg::autoposter_source
     * @since 2.0.0
     */
    void start_autoposter(autoposter_source* source, const post_stats_completion_t& callback, time_t interval = TOPGG_AUTOPOSTER_MIN_INTERVAL);

    /**
     * @brief Starts autoposting your Discord bot's statistics using a custom data source.
     *
     * @param source A pointer to a autoposter statistics source located in the heap memory. This pointer must be allocated with new, and it will be deleted once the autoposter thread gets stopped.
     * @param interval The interval between posting in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_stats
     * @see topgg::client::stop_autoposter
     * @see topgg::autoposter_source
     * @since 2.0.0
     */
    void start_autoposter(autoposter_source* source, time_t interval = TOPGG_AUTOPOSTER_MIN_INTERVAL);

    /**
     * @brief Prematurely stops the autoposter. Calling this function is usually unnecessary as this function will be called in the destructor.
     *
     * @note This function has no effect if the autoposter is already stopped.
     * @see topgg::client::post_stats
     * @since 2.0.0
     */
    void stop_autoposter() noexcept;

    /**
     * @brief The destructor. Stops the autoposter if it's running.
     */
    ~client();

    friend class bot_query;
  };
}; // namespace topgg
