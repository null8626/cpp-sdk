/**
 * @module topgg
 * @file client.h
 * @brief The official C++ wrapper for the Top.gg API.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2025 Top.gg & null8626
 * @date 2025-02-25
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
  using get_bot_completion_event = std::function<void(const result<bot>&)>;

  /**
   * @brief The callback function to call when get_server_count completes.
   *
   * @see topgg::client::get_server_count
   * @since 3.0.0
   */
  using get_server_count_completion_event = std::function<void(const result<std::optional<size_t>>&)>;

  /**
   * @brief The callback function to call when get_voters completes.
   *
   * @see topgg::client::get_voters
   * @since 2.0.0
   */
  using get_voters_completion_event = std::function<void(const result<std::vector<voter>>&)>;

  /**
   * @brief The callback function to call when has_voted completes.
   *
   * @see topgg::client::has_voted
   * @since 2.0.0
   */
  using has_voted_completion_event = std::function<void(const result<bool>&)>;

  /**
   * @brief The callback function to call when is_weekend completes.
   *
   * @see topgg::client::is_weekend
   * @since 2.0.0
   */
  using is_weekend_completion_event = std::function<void(const result<bool>&)>;
  
  /**
   * @brief The callback function to call when post_server_count completes.
   *
   * @see topgg::client::post_server_count
   * @since 3.0.0
   */
  using post_server_count_completion_event = std::function<void(const bool)>;

  /**
   * @brief The callback function to call after every request to the API, successful or not.
   *
   * @see topgg::client::start_autoposter
   * @see topgg::client::stop_autoposter
   * @since 3.0.0
   */
  using autopost_completion_event = std::function<void(const std::optional<size_t>&)>;
  
  /**
   * @brief Main client class that lets you make HTTP requests with the Top.gg API.
   *
   * @since 2.0.0
   */
  class TOPGG_EXPORT client {
    std::multimap<std::string, std::string> m_headers;
    std::string m_token;
    std::string m_id;
    dpp::cluster& m_cluster;
    dpp::timer m_autoposter_timer;

    template<typename T>
    void basic_request(const std::string& url, const std::function<void(const result<T>&)>& callback, std::function<T(const dpp::json&)>&& conversion_fn) {
      m_cluster.request(TOPGG_BASE_URL + url, dpp::m_get, [callback, conversion_fn_in = std::move(conversion_fn)](const auto& response) { callback(result<T>{response, conversion_fn_in}); }, "", "application/json", m_headers);
    }

    size_t get_server_count();
    void post_server_count_inner(const size_t server_count, dpp::http_completion_event callback);
    
  public:
    client() = delete;

    /**
     * @brief Constructs the client class.
     *
     * @param cluster A pointer to the bot's D++ cluster using this library.
     * @param token The Top.gg API token to use.
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
     * @brief Fetches a listed Discord bot from a Discord ID.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client.get_bot(264811613708746752, [](const auto& result) {
     *   try {
     *     const auto topgg_bot = result.get();
     *
     *     std::cout << topgg_bot.username << std::endl;
     *   } catch (const std::exception& exc) {
     *     std::cerr << "error: " << exc.what() << std::endl;
     *   }
     * });
     * ```
     *
     * @param bot_id The Discord bot ID to fetch from.
     * @param callback The callback function to call when get_bot completes.
     * @note For its C++20 coroutine counterpart, see co_get_bot.
     * @see topgg::result
     * @see topgg::bot
     * @see topgg::client::co_get_bot
     * @since 2.0.0
     */
    void get_bot(const dpp::snowflake bot_id, const get_bot_completion_event& callback);

#ifdef DPP_CORO
    /**
     * @brief Fetches a listed Discord bot from a Discord ID through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * try {
     *   const auto topgg_bot = co_await topgg_client.co_get_bot(264811613708746752);
     *
     *   std::cout << topgg_bot.username << std::endl;
     * } catch (const std::exception& exc) {
     *   std::cerr << "error: " << exc.what() << std::endl;
     * }
     * ```
     *
     * @param bot_id The Discord bot ID to fetch from.
     * @throw topgg::internal_server_error Thrown when the client receives an unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Thrown when its known that the client uses an invalid Top.gg API token.
     * @throw topgg::not_found Thrown when such query does not exist.
     * @throw topgg::ratelimited Thrown when the client gets ratelimited from sending more HTTP requests.
     * @throw dpp::http_error Thrown when an unexpected HTTP exception has occured.
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
     * @brief Queries/searches through the Top.gg database to look for matching listed Discord bots.
     *
     * C++17 example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client
     *   .get_bots()
     *   .limit(250)
     *   .skip(50)
     *   .name("shiro")
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
     * C++20 example:
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
     *     .name("shiro")
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
     * @return bot_query An object for configuring the query in get_bots before being sent to the Top.gg API.
     * @see topgg::bot_query
     * @since 2.0.1
     */
    inline bot_query get_bots() noexcept {
      return bot_query{this};
    }

    /**
     * @brief Fetches your Discord bot's posted server count.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client.get_server_count([](const auto& result) {
     *   try {
     *     auto server_count = result.get();
     *
     *     std::cout << server_count.value_or(0) << std::endl;
     *   } catch (const std::exception& exc) {
     *     std::cerr << "error: " << exc.what() << std::endl;
     *   }
     * });
     * ```
     *
     * @param callback The callback function to call when get_server_count completes.
     * @note For its C++20 coroutine counterpart, see co_get_server_count.
     * @see topgg::result
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_get_server_count
     * @since 3.0.0
     */
    void get_server_count(const get_server_count_completion_event& callback);

#ifdef DPP_CORO
    /**
     * @brief Fetches your Discord bot's posted server count through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * try {
     *   const auto server_count = co_await topgg_client.co_get_server_count();
     *
     *   std::cout << server_count.value_or(0) << std::endl;
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
     * @return co_await to retrieve an optional size_t if successful
     * @note For its C++17 callback-based counterpart, see get_server_count.
     * @see topgg::async_result
     * @see topgg::client::start_autoposter
     * @see topgg::client::get_server_count
     * @since 3.0.0
     */
    topgg::async_result<std::optional<size_t>> co_get_server_count();
#endif

    /**
     * @brief Fetches your Discord bot's last 1000 unique voters.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client.get_voters([](const auto& result) {
     *   try {
     *     auto voters = result.get();
     *
     *     for (auto& voter: voters) {
     *       std::cout << voter.username << std::endl;
     *     }
     *   } catch (const std::exception& exc) {
     *     std::cerr << "error: " << exc.what() << std::endl;
     *   }
     * });
     * ```
     *
     * @param callback The callback function to call when get_voters completes.
     * @note For its C++20 coroutine counterpart, see co_get_voters.
     * @see topgg::result
     * @see topgg::voter
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_get_voters
     * @since 2.0.0
     */
    void get_voters(const get_voters_completion_event& callback);

#ifdef DPP_CORO
    /**
     * @brief Fetches your Discord bot's last 1000 unique voters through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * try {
     *   const auto voters = co_await topgg_client.co_get_voters();
     *
     *   for (const auto& voter: voters) {
     *     std::cout << voter.username << std::endl;
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
     * @return co_await to retrieve a vector of topgg::voter if successful
     * @note For its C++17 callback-based counterpart, see get_voters.
     * @see topgg::async_result
     * @see topgg::voter
     * @see topgg::client::start_autoposter
     * @see topgg::client::get_voters
     * @since 2.0.0
     */
    topgg::async_result<std::vector<voter>> co_get_voters();
#endif

    /**
     * @brief Checks if the specified user has voted your Discord bot.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client.has_voted(661200758510977084, [](const auto& result) {
     *   try {
     *     if (result.get()) {
     *       std::cout << "checks out" << std::endl;
     *     }
     *   } catch (const std::exception& exc) {
     *     std::cerr << "error: " << exc.what() << std::endl;
     *   }
     * });
     * ```
     *
     * @param user_id The Discord user ID to check from.
     * @param callback The callback function to call when has_voted completes.
     * @note For its C++20 coroutine counterpart, see co_has_voted.
     * @see topgg::result
     * @see topgg::client::start_autoposter
     * @note For its C++20 coroutine counterpart, see co_has_voted.
     * @since 2.0.0
     */
    void has_voted(const dpp::snowflake user_id, const has_voted_completion_event& callback);

#ifdef DPP_CORO
    /**
     * @brief Checks if the specified user has voted your Discord bot through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * try {
     *   const auto voted = co_await topgg_client.co_has_voted(661200758510977084);
     *
     *   if (voted) {
     *     std::cout << "checks out" << std::endl;
     *   }
     * } catch (const std::exception& exc) {
     *   std::cerr << "error: " << exc.what() << std::endl;
     * }
     * ```
     *
     * @param user_id The Discord user ID to check from.
     * @throw topgg::internal_server_error Thrown when the client receives an unexpected error from Top.gg's end.
     * @throw topgg::invalid_token Thrown when its known that the client uses an invalid Top.gg API token.
     * @throw topgg::not_found Thrown when such query does not exist.
     * @throw topgg::ratelimited Thrown when the client gets ratelimited from sending more HTTP requests.
     * @throw dpp::http_error Thrown when an unexpected HTTP exception has occured.
     * @return co_await to retrieve a bool if successful
     * @note For its C++17 callback-based counterpart, see has_voted.
     * @see topgg::async_result
     * @see topgg::client::start_autoposter
     * @see topgg::client::has_voted
     * @since 2.0.0
     */
    topgg::async_result<bool> co_has_voted(const dpp::snowflake user_id);
#endif

    /**
     * @brief Checks if the weekend multiplier is active.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client.is_weekend([](const auto& result) {
     *   try {
     *     if (result.get()) {
     *       std::cout << "the weekend multiplier is active" << std::endl;
     *     }
     *   } catch (const std::exception& exc) {
     *     std::cerr << "error: " << exc.what() << std::endl;
     *   }
     * });
     * ```
     *
     * @param callback The callback function to call when is_weekend completes.
     * @note For its C++20 coroutine counterpart, see co_is_weekend.
     * @see topgg::result
     * @see topgg::client::co_is_weekend
     * @since 2.0.0
     */
    void is_weekend(const is_weekend_completion_event& callback);

#ifdef DPP_CORO
    /**
     * @brief Checks if the weekend multiplier is active through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * try {
     *   const auto is_weekend = co_await topgg_client.co_is_weekend();
     *
     *   if (is_weekend) {
     *     std::cout << "the weekend multiplier is active" << std::endl;
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
     * @return co_await to retrieve a bool if successful
     * @note For its C++17 callback-based counterpart, see is_weekend.
     * @see topgg::async_result
     * @see topgg::client::is_weekend
     * @since 2.0.0
     */
    topgg::async_result<bool> co_is_weekend();
#endif

    /**
     * @brief Manually posts your Discord bot's server count using data directly from your D++ cluster instance.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * topgg_client.post_server_count([](const auto success) {
     *   if (success) {
     *     std::cout << "stats posted!" << std::endl;
     *   }
     * });
     * ```
     *
     * @param callback The callback function to call when post_server_count completes.
     * @note For its C++20 coroutine counterpart, see co_post_server_count.
     * @see topgg::result
     * @see topgg::client::start_autoposter
     * @see topgg::client::co_post_server_count
     * @since 3.0.0
     */
    void post_server_count(const post_server_count_completion_event& callback);

#ifdef DPP_CORO
    /**
     * @brief Manually posts your Discord bot's server count using data directly from your D++ cluster instance through a C++20 coroutine.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * const auto success = co_await topgg_client.co_post_server_count();
     *
     * if (success) {
     *   std::cout << "stats posted!" << std::endl;
     * }
     * ```
     *
     * @return co_await to retrieve a bool
     * @note For its C++17 callback-based counterpart, see post_server_count.
     * @see topgg::client::start_autoposter
     * @see topgg::client::post_server_count
     * @since 3.0.0
     */
    dpp::async<bool> co_post_server_count();
#endif

    /**
     * @brief Starts autoposting your bot's server count using data directly from your D++ cluster instance.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * bot.start_autoposter([](const auto& result) {
     *   if (result) {
     *     std::cout << "Successfully posted " << *result << " servers to the Top.gg API!" << std::endl;
     *   }
     * });
     * ```
     *
     * @param callback The callback function to call after every request to the API, successful or not.
     * @param delay The minimum delay between post requests in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_server_count
     * @see topgg::client::stop_autoposter
     * @see topgg::autopost_completion_event
     * @since 2.0.0
     */
    void start_autoposter(const autopost_completion_event& callback, time_t delay = TOPGG_AUTOPOSTER_MIN_DELAY);

    /**
     * @brief Starts autoposting your bot's server count using data directly from your D++ cluster instance.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * bot.start_autoposter();
     * ```
     *
     * @param delay The minimum delay between post requests in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_server_count
     * @see topgg::client::stop_autoposter
     * @since 2.0.0
     */
    void start_autoposter(time_t delay = TOPGG_AUTOPOSTER_MIN_DELAY);

    /**
     * @brief Starts autoposting your bot's server count using a custom data source.
     *
     * Example:
     *
     * ```cpp
     * class my_autoposter_source: private topgg::autoposter_source {
     * public:
     *   virtual size_t get_server_count(dpp::cluster& bot) {
     *     return ...;
     *   }
     * };
     * 
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     * 
     * topgg_client.start_autoposter(reinterpret_cast<topgg::autoposter_source*>(new my_autoposter_source), [](const auto& result) {
     *   if (result) {
     *     std::cout << "Successfully posted " << *result << " servers to the Top.gg API!" << std::endl;
     *   }
     * });
     * ```
     *
     * @param source A pointer to an autoposter source located in the heap memory. This pointer must be allocated with new, and it will be deleted once the autoposter thread gets stopped.
     * @param callback The callback function to call after every request to the API, successful or not.
     * @param delay The minimum delay between post requests in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_server_count
     * @see topgg::client::stop_autoposter
     * @see topgg::autopost_completion_event
     * @see topgg::autoposter_source
     * @since 2.0.0
     */
    void start_autoposter(autoposter_source* source, const autopost_completion_event& callback, time_t delay = TOPGG_AUTOPOSTER_MIN_DELAY);

    /**
     * @brief Starts autoposting your bot's server count using a custom data source.
     *
     * Example:
     *
     * ```cpp
     * class my_autoposter_source: private topgg::autoposter_source {
     * public:
     *   virtual size_t get_server_count(dpp::cluster& bot) {
     *     return ...;
     *   }
     * };
     * 
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     * 
     * topgg_client.start_autoposter(reinterpret_cast<topgg::autoposter_source*>(new my_autoposter_source));
     * ```
     *
     * @param source A pointer to an autoposter source located in the heap memory. This pointer must be allocated with new, and it will be deleted once the autoposter thread gets stopped.
     * @param delay The minimum delay between post requests in seconds. Defaults to 15 minutes.
     * @note This function has no effect if the autoposter is already running.
     * @see topgg::client::post_server_count
     * @see topgg::client::stop_autoposter
     * @see topgg::autoposter_source
     * @since 3.0.0
     */
    void start_autoposter(autoposter_source* source, time_t delay = TOPGG_AUTOPOSTER_MIN_DELAY);
    
    /**
     * @brief Prematurely stops the autoposter. Calling this function is usually unnecessary as this function is called later in the destructor.
     *
     * Example:
     *
     * ```cpp
     * dpp::cluster bot{"your bot token"};
     * topgg::client topgg_client{bot, "your top.gg token"};
     *
     * bot.start_autoposter();
     *
     * // ...
     *
     * bot.stop_autoposter();
     * ```
     *
     * @note This function has no effect if the autoposter is already stopped.
     * @see topgg::client::post_server_count
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