#include <topgg/topgg.h>

using topgg::client;

// clang-format off
static constexpr unsigned char g_base64_decoding_table[] = {
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,
  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
  37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64
};
// clang-format on

static bool base64_decode(const std::string& input, std::string& output) {
  const auto input_size{input.size()};

  if (input_size % 4 != 0) {
    return false;
  }

  auto output_size{input_size / 4 * 3};

  if (input_size >= 1 && input[input_size - 1] == '=') {
    output_size--;
  }

  if (input_size >= 2 && input[input_size - 2] == '=') {
    output_size--;
  }

  output.resize(output_size);

  uint32_t a, b, c, d, triple{};

  for (size_t i = 0, j = 0; i < input_size;) {
    a = input[i] == '=' ? 0 & i++ : g_base64_decoding_table[static_cast<int>(input[i++])];
    b = input[i] == '=' ? 0 & i++ : g_base64_decoding_table[static_cast<int>(input[i++])];
    c = input[i] == '=' ? 0 & i++ : g_base64_decoding_table[static_cast<int>(input[i++])];
    d = input[i] == '=' ? 0 & i++ : g_base64_decoding_table[static_cast<int>(input[i++])];

    triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

    if (j < output_size) {
      output[j++] = (triple >> 2 * 8) & 0xFF;
    }

    if (j < output_size) {
      output[j++] = (triple >> 1 * 8) & 0xFF;
    }

    if (j < output_size) {
      output[j++] = (triple >> 0 * 8) & 0xFF;
    }
  }

  return true;
}

static dpp::json parse_api_token(const std::string& token) {
  const auto first_pos{token.find('.')};

  if (first_pos != std::string::npos) {
    const auto first_token_slice{token.substr(first_pos + 1)};
    const auto second_pos{first_token_slice.find('.')};

    if (second_pos != std::string::npos) {
      auto base64_input{first_token_slice.substr(0, second_pos)};
      const auto additional_equals{4 - (base64_input.length() % 4)};

      for (size_t i{}; i < additional_equals; i++) {
        base64_input.push_back('=');
      }

      std::string base64_decoded{};

      if (base64_decode(base64_input, base64_decoded)) {
        return dpp::json::parse(base64_decoded);
      }
    }
  }

  throw std::invalid_argument{"Got a malformed API token."};
}

client::client(dpp::cluster& cluster, const std::string& token)
  : m_cluster(cluster), m_autoposter_timer(0) {
  const auto token_data{parse_api_token(token)};

  m_id = _TOPGG_SNOWFLAKE_FROM_JSON(token_data, id);
  
  m_headers.insert(std::pair("Authorization", "Bearer " + token));
  m_headers.insert(std::pair("Content-Type", "application/json"));
  m_headers.insert(std::pair("User-Agent", "topgg (https://github.com/top-gg-community/cpp-sdk) D++"));
}

void client::get_bot(const dpp::snowflake bot_id, const topgg::get_bot_completion_t& callback) {
  basic_request<topgg::bot>(dpp::m_get, "/bots/" + bot_id.str(), callback, [](const auto& j) {
    return topgg::bot{j};
  });
}

#ifdef DPP_CORO
topgg::async_result<topgg::bot> client::co_get_bot(const dpp::snowflake bot_id) {
  return topgg::async_result<topgg::bot>{[this, bot_id]<typename C>(C&& cc) { return get_bot(bot_id, std::forward<C>(cc)); }};
}
#endif

void client::get_user(const dpp::snowflake user_id, const topgg::get_user_completion_t& callback) {
  basic_request<topgg::user>(dpp::m_get, "/users/" + user_id.str(), callback, [](const auto& j) {
    return topgg::user{j};
  });
}

#ifdef DPP_CORO
topgg::async_result<topgg::user> client::co_get_user(const dpp::snowflake user_id) {
  return topgg::async_result<topgg::user>{ [this, user_id] <typename C> (C&& cc) { return get_user(user_id, std::forward<C>(cc)); }};
}
#endif

topgg::stats client::get_stats() {
#ifdef __TOPGG_TESTING__
  return topgg::stats{2};
#else
  return topgg::stats{m_cluster};
#endif
}

void client::post_stats_inner(const size_t server_count, const dpp::http_completion_event callback) {
  dpp::json j{};
  j["server_count"] = server_count;

  request(dpp::m_post, "/bots/stats", callback, j.dump());
}

void client::post_stats(const topgg::post_stats_completion_t& callback) {
  const auto stats{get_stats()};
  const auto server_count{stats.server_count().value_or(0)};

  if (server_count <= 0) {
    return callback(false);
  }

  post_stats_inner(server_count, [callback](const auto& response) {
    callback(response.error == dpp::h_success && response.status < 400);
  });
}

#ifdef DPP_CORO
dpp::async<bool> client::co_post_stats() {
  return dpp::async<bool>{[this]<typename C>(C&& cc) { return post_stats(std::forward<C>(cc)); }};
}
#endif

void client::get_stats(const topgg::get_stats_completion_t& callback) {
  basic_request<topgg::stats>(dpp::m_get, "/bots/stats", callback, [](const auto& j) {
    return topgg::stats{j};
  });
}

#ifdef DPP_CORO
topgg::async_result<topgg::stats> client::co_get_stats() {
  return topgg::async_result<topgg::stats>{[this]<typename C>(C&& cc) { return get_stats(std::forward<C>(cc)); }};
}
#endif

void client::get_voters(size_t page, const topgg::get_voters_completion_t& callback) {
  if (page < 1) {
    page = 1;
  }

  basic_request<std::vector<topgg::voter>>(dpp::m_get, "/bots/" + m_id.str() + "/votes?page=" + std::to_string(page), callback, [](const auto& j) {
    std::vector<topgg::voter> voters{};

    for (const auto& part: j) {
      voters.push_back(topgg::voter{part});
    }

    return voters;
  });
}

void client::get_voters(const topgg::get_voters_completion_t& callback) {
  get_voters(1, callback);
}

#ifdef DPP_CORO
topgg::async_result<std::vector<topgg::voter>> client::co_get_voters(size_t page) {
  return topgg::async_result<std::vector<topgg::voter>>{[this, page]<typename C>(C&& cc) { return get_voters(page, std::forward<C>(cc)); }};
}
#endif

void client::has_voted(const dpp::snowflake user_id, const topgg::has_voted_completion_t& callback) {
  basic_request<bool>(dpp::m_get, "/bots/check?userId=" + user_id.str(), callback, [](const auto& j) {
    return j["voted"].template get<uint8_t>() != 0;
  });
}

#ifdef DPP_CORO
topgg::async_result<bool> client::co_has_voted(const dpp::snowflake user_id) {
  return topgg::async_result<bool>{[user_id, this]<typename C>(C&& cc) { return has_voted(user_id, std::forward<C>(cc)); }};
}
#endif

void client::is_weekend(const topgg::is_weekend_completion_t& callback) {
  basic_request<bool>(dpp::m_get, "/weekend", callback, [](const auto& j) {
    return j["is_weekend"].template get<bool>();
  });
}

#ifdef DPP_CORO
topgg::async_result<bool> client::co_is_weekend() {
  return topgg::async_result<bool>{[this]<typename C>(C&& cc) { return is_weekend(std::forward<C>(cc)); }};
}
#endif

void client::start_autoposter(const topgg::post_stats_completion_t& callback, time_t interval) {
  start_autoposter([this](TOPGG_UNUSED const auto&) {
    return get_stats();
  }, callback, interval);
}

void client::start_autoposter(const topgg::custom_autopost_callback_t& stats_callback, const topgg::post_stats_completion_t& post_callback, time_t interval) {
  if (interval < TOPGG_AUTOPOSTER_MIN_INTERVAL) {
    interval = TOPGG_AUTOPOSTER_MIN_INTERVAL;
  }

  /**
   * Create a D++ timer, this is managed by the D++ cluster and ticks every n seconds.
   * It can be stopped at any time without blocking, and does not need to create extra threads.
   */
  if (!m_autoposter_timer) {
    // clang-format off
    m_autoposter_timer = m_cluster.start_timer([this, stats_callback, post_callback](TOPGG_UNUSED dpp::timer) {
      const auto stats{stats_callback(m_cluster)};
      const auto server_count{stats.server_count().value_or(0)};

      if (server_count > 0) {
        post_stats_inner(server_count, [post_callback](TOPGG_UNUSED const auto& response) {
          post_callback(response.error == dpp::h_success && response.status < 400);
        });
      }
    }, interval);
    // clang-format on
  }
}

void client::start_autoposter(const time_t interval) {
  start_autoposter([](TOPGG_UNUSED const auto) {}, interval);
}

void client::start_autoposter(topgg::autoposter_source* source, const topgg::post_stats_completion_t& callback, time_t interval) {
  start_autoposter([source](auto& cluster) {
    return source->get_stats(cluster);
  }, callback, interval);
}

void client::start_autoposter(topgg::autoposter_source* source, time_t interval) {
  start_autoposter(source, [](TOPGG_UNUSED const auto) {}, interval);
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