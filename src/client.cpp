#include <topgg/topgg.h>

using topgg::client;

client::client(dpp::cluster& cluster, const std::string& token)
  : m_token(token), m_cluster(cluster), m_autoposter_timer(0) {
  m_headers.insert(std::pair("Authorization", token));
  m_headers.insert(std::pair("Content-Type", "application/json"));
  m_headers.insert(std::pair("User-Agent", "topgg (https://github.com/top-gg-community/cpp-sdk) D++"));
}

void client::get_bot(const dpp::snowflake bot_id, const topgg::get_bot_completion_event& callback) {
  basic_request<topgg::bot>("/bots/" + std::to_string(bot_id), callback, [](const auto& j) {
    return topgg::bot{j};
  });
}

#ifdef DPP_CORO
topgg::async_result<topgg::bot> client::co_get_bot(const dpp::snowflake bot_id) {
  return topgg::async_result<topgg::bot>{[this, bot_id]<typename C>(C&& cc) { return get_bot(bot_id, std::forward<C>(cc)); }};
}
#endif

size_t client::get_server_count() {
#ifdef __TOPGG_TESTING__
  return 2;
#else
  size_t server_count{};

  for (auto& s: m_cluster.get_shards()) {
    server_count += s.second->get_guild_count();
  }

  return server_count;
#endif
}

void client::post_server_count_inner(const size_t server_count, dpp::http_completion_event callback) {
  auto headers{m_headers};
  dpp::json j{};

  j["server_count"] = server_count;

  m_cluster.request(TOPGG_BASE_URL "/bots/stats", dpp::m_post, callback, j.dump(), "application/json", headers);
}

void client::post_server_count(const topgg::post_server_count_completion_event& callback) {
  const auto server_count{get_server_count()};

  if (server_count == 0) {
    return callback(false);
  }

  post_server_count_inner(server_count, [callback](const auto& response) {
    callback(response.error == dpp::h_success && response.status < 400);
  });
}

#ifdef DPP_CORO
dpp::async<bool> client::co_post_server_count() {
  return dpp::async<bool>{[this]<typename C>(C&& cc) { return post_server_count(std::forward<C>(cc)); }};
}
#endif

void client::get_server_count(const topgg::get_server_count_completion_event& callback) {
  basic_request<std::optional<size_t>>("/bots/stats", callback, [](const auto& j) {
    std::optional<size_t> server_count{};

    try {
      *server_count = j["server_count"].template get<size_t>();
    } catch (const std::exception&) {}

    return server_count;
  });
}

#ifdef DPP_CORO
topgg::async_result<std::optional<size_t>> client::co_get_server_count() {
  return topgg::async_result<std::optional<size_t>>{[this]<typename C>(C&& cc) { return get_server_count(std::forward<C>(cc)); }};
}
#endif

void client::get_voters(size_t page, const topgg::get_voters_completion_event& callback) {
  if (page < 1) {
    page = 1;
  }

  basic_request<std::vector<topgg::voter>>("/bots/votes?page=" + std::to_string(page), callback, [](const auto& j) {
    std::vector<topgg::voter> voters{};

    for (const auto& part: j) {
      voters.push_back(topgg::voter{part});
    }

    return voters;
  });
}

void client::get_voters(const topgg::get_voters_completion_event& callback) {
  get_voters(1, callback);
}

#ifdef DPP_CORO
topgg::async_result<std::vector<topgg::voter>> client::co_get_voters(size_t page) {
  return topgg::async_result<std::vector<topgg::voter>>{[this, page]<typename C>(C&& cc) { return get_voters(page, std::forward<C>(cc)); }};
}
#endif

void client::has_voted(const dpp::snowflake user_id, const topgg::has_voted_completion_event& callback) {
  basic_request<bool>("/bots/check?userId=" + std::to_string(user_id), callback, [](const auto& j) {
    return j["voted"].template get<uint8_t>() != 0;
  });
}

#ifdef DPP_CORO
topgg::async_result<bool> client::co_has_voted(const dpp::snowflake user_id) {
  return topgg::async_result<bool>{[user_id, this]<typename C>(C&& cc) { return has_voted(user_id, std::forward<C>(cc)); }};
}
#endif

void client::is_weekend(const topgg::is_weekend_completion_event& callback) {
  basic_request<bool>("/weekend", callback, [](const auto& j) {
    return j["is_weekend"].template get<bool>();
  });
}

#ifdef DPP_CORO
topgg::async_result<bool> client::co_is_weekend() {
  return topgg::async_result<bool>{[this]<typename C>(C&& cc) { return is_weekend(std::forward<C>(cc)); }};
}
#endif

void client::start_autoposter(const topgg::autopost_completion_event& callback, time_t interval) {
  if (interval < TOPGG_AUTOPOSTER_MIN_INTERVAL) {
    interval = TOPGG_AUTOPOSTER_MIN_INTERVAL;
  }

  /**
   * Create a D++ timer, this is managed by the D++ cluster and ticks every n seconds.
   * It can be stopped at any time without blocking, and does not need to create extra threads.
   */
  if (!m_autoposter_timer) {
    // clang-format off
    m_autoposter_timer = m_cluster.start_timer([this, callback](TOPGG_UNUSED dpp::timer) {
      const auto server_count{get_server_count()};

      if (server_count > 0) {
        post_server_count_inner(server_count, [callback, server_count](const auto& response) {
          if (response.error == dpp::h_success && response.status < 400) {
            callback(std::optional{server_count});
          } else {
            callback(std::nullopt);
          }
        });
      }
    }, interval);
    // clang-format on
  }
}

void client::start_autoposter(const time_t interval) {
  start_autoposter([](TOPGG_UNUSED const auto&) {}, interval);
}

void client::start_autoposter(topgg::autoposter_source* source, const topgg::autopost_completion_event& callback, time_t interval) {
  if (!m_autoposter_timer) {
    if (interval < TOPGG_AUTOPOSTER_MIN_INTERVAL) {
      interval = TOPGG_AUTOPOSTER_MIN_INTERVAL;
    }

    // clang-format off
    m_autoposter_timer = m_cluster.start_timer([this, callback, source](TOPGG_UNUSED dpp::timer) {
      const auto server_count{source->get_server_count(m_cluster)};

      if (server_count > 0) {
        post_server_count_inner(server_count, [callback, server_count](const auto& response) {
          if (response.error == dpp::h_success && response.status < 400) {
            callback(std::optional{server_count});
          } else {
            callback(std::nullopt);
          }
        });
      }
    }, interval, [source](TOPGG_UNUSED dpp::timer) { delete source; });
    // clang-format on
  }
}

void client::start_autoposter(topgg::autoposter_source* source, time_t interval) {
  start_autoposter(source, [](TOPGG_UNUSED const auto&) {}, interval);
}

void client::stop_autoposter() noexcept {
  if (m_autoposter_timer) {
    m_cluster.stop_timer(m_autoposter_timer);
    m_autoposter_timer = 0;
  }
}

client::~client() {
  stop_autoposter();
}