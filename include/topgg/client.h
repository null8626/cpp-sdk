/**
 * @module topgg
 * @file client.h
 * @brief The community-maintained C++ library for Top.gg.
 * @authors Top.gg, null8626
 * @copyright Copyright (c) 2024-2026 Top.gg & null8626
 * @date 2026-03-10
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
   * @brief Main client class that lets you make HTTP requests with the Top.gg API.
   *
   * @since 2.0.0
   */
  class TOPGG_EXPORT client {
    std::multimap<std::string, std::string> m_headers;
    std::string m_token;
    dpp::cluster& m_cluster;

    template<typename T>
    void basic_request(const std::string& url, const std::function<void(const result<T>&)>& callback, std::function<T(const dpp::json&)>&& conversion_fn) {
      m_cluster.request("https://top.gg/api" + url, dpp::m_get, [callback, conversion_fn_in = std::move(conversion_fn)](const auto& response) { callback(result<T>{response, conversion_fn_in}); }, "", "application/json", m_headers);
    }
    
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
  };
}; // namespace topgg
