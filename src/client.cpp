#include <topgg/topgg.h>

using topgg::client;

client::client(dpp::cluster& cluster, const std::string& token): m_token(token), m_cluster(cluster), m_autoposter_timer(0) {
  m_headers.insert(std::pair("Authorization", "Bearer " + token));
  m_headers.insert(std::pair("Connection", "close"));
  m_headers.insert(std::pair("Content-Type", "application/json"));
  m_headers.insert(std::pair("User-Agent", "topgg (https://github.com/top-gg-community/cpp-sdk) D++"));
}

void client::get_bot(const dpp::snowflake bot_id, const topgg::get_bot_completion_t& callback) {
  basic_request<topgg::bot>("/bots/" + std::to_string(bot_id), callback, [](const auto& j) {
    return topgg::bot{j};
  });
}

#ifdef DPP_CORO
topgg::async_result<topgg::bot> client::co_get_bot(const dpp::snowflake bot_id) {
  return topgg::async_result<topgg::bot>{ [this, bot_id] <typename C> (C&& cc) { return get_bot(bot_id, std::forward<C>(cc)); }};
}
#endif

void client::get_user(const dpp::snowflake user_id, const topgg::get_user_completion_t& callback) {
  basic_request<topgg::user>("/users/" + std::to_string(user_id), callback, [](const auto& j) {
    return topgg::user{j};
  });
}

#ifdef DPP_CORO
topgg::async_result<topgg::user> client::co_get_user(const dpp::snowflake user_id) {
  return topgg::async_result<topgg::user>{ [this, user_id] <typename C> (C&& cc) { return get_user(user_id, std::forward<C>(cc)); }};
}
#endif

size_t client::get_server_count() {
  size_t server_count{};

  for (auto& s: m_cluster.get_shards()) {
    server_count += s.second->get_guild_count();
  }

  return server_count;
}

void client::post_server_count_inner(const size_t server_count, dpp::http_completion_event callback) {
  std::multimap<std::string, std::string> headers{m_headers};
  dpp::json j{};

  j["server_count"] = server_count;

  const auto s_json{j.dump()};
  headers.insert(std::pair("Content-Length", std::to_string(s_json.size())));

  m_cluster.request(TOPGG_BASE_URL "/bots/stats", dpp::m_post, callback, s_json, "application/json", headers);
}

void client::post_server_count(const topgg::post_server_count_completion_t& callback)  {
  post_server_count_inner([callback](const auto& response) {
    callback(response.error == dpp::h_success && response.status < 400);
  });
}

#ifdef DPP_CORO
dpp::async<bool> client::co_post_server_count() {
  return dpp::async<bool>{ [this] <typename C> (C&& cc) { return post_server_count(std::forward<C>(cc)); }};
}
#endif

void client::get_server_count(const topgg::get_server_count_completion_t& callback) {
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
  return topgg::async_result<std::optional<size_t>>{ [this] <typename C> (C&& cc) { return get_server_count(std::forward<C>(cc)); }};
}
#endif

void client::get_voters(const topgg::get_voters_completion_t& callback) {
  basic_request<std::vector<topgg::voter>>("/bots/votes", callback, [](const auto& j) {
    std::vector<topgg::voter> voters{};

    for (const auto& part: j) {
      voters.push_back(topgg::voter{part});
    }

    return voters;
  });
}

#ifdef DPP_CORO
topgg::async_result<std::vector<topgg::voter>> client::co_get_voters() {
  return topgg::async_result<std::vector<topgg::voter>>{ [this] <typename C> (C&& cc) { return get_voters(std::forward<C>(cc)); }};
}
#endif


void client::has_voted(const dpp::snowflake user_id, const topgg::has_voted_completion_t& callback) {
  basic_request<bool>("/bots/votes?userId=" + std::to_string(user_id), callback, [](const auto& j) {
    return j["voted"].template get<uint8_t>() != 0;
  });
}

#ifdef DPP_CORO
topgg::async_result<bool> client::co_has_voted(const dpp::snowflake user_id) {
  return topgg::async_result<bool>{ [user_id, this] <typename C> (C&& cc) { return has_voted(user_id, std::forward<C>(cc)); }};
}
#endif

void client::is_weekend(const topgg::is_weekend_completion_t& callback) {
  basic_request<bool>("/weekend", callback, [](const auto& j) {
    return j["is_weekend"].template get<bool>();
  });
}

#ifdef DPP_CORO
topgg::async_result<bool> client::co_is_weekend() {
  return topgg::async_result<bool>{ [this] <typename C> (C&& cc) { return is_weekend(std::forward<C>(cc)); }};
}
#endif

void client::start_autoposter(const time_t delay) {
  /**
   * Check the timer duration is not less than 15 minutes
   */
  if (delay < 15 * 60) {
    throw std::invalid_argument{"Delay mustn't be shorter than 15 minutes."};
  }
  
  /**
   * Create a D++ timer, this is managed by the D++ cluster and ticks every n seconds.
   * It can be stopped at any time without blocking, and does not need to create extra threads.
   */
  else if (!m_autoposter_timer) {
    m_autoposter_timer = m_cluster.start_timer([this](TOPGG_UNUSED dpp::timer) {
      post_server_count_inner([](TOPGG_UNUSED const auto&) {});
    }, delay);
  }
}

void client::start_autoposter(topgg::autoposter_source* source, const time_t delay) {
  if (delay < 15 * 60) {
    delete source;
    throw std::invalid_argument{"Delay mustn't be shorter than 15 minutes."};
  } else if (!m_autoposter_timer) {
    m_autoposter_timer = m_cluster.start_timer([this, source](TOPGG_UNUSED dpp::timer) {
      post_server_count_inner(source->get_server_count(m_cluster), [](TOPGG_UNUSED const auto&) {});
    }, delay, [source](TOPGG_UNUSED dpp::timer) {
      delete source;
    });
  }
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