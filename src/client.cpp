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

static std::string id_from_bot_token(std::string bot_token) {
  const auto pos{bot_token.find('.')};

  if (pos != std::string::npos) {
    std::string decoded_base64{};
    auto base64_input{bot_token.substr(0, pos)};
    const auto additional_equals{4 - (base64_input.length() % 4)};

    for (size_t j{}; j < additional_equals; j++) {
      base64_input.push_back('=');
    }

    if (base64_decode(base64_input, decoded_base64)) {
      return decoded_base64;
    }
  }

  throw std::invalid_argument{"Got a malformed Discord Bot token."};
}

client::client(dpp::cluster& cluster, const std::string& token)
  : m_token(token), m_cluster(cluster), m_autoposter_timer(0) {
  m_id = id_from_bot_token(cluster.token);
  
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

  basic_request<std::vector<topgg::voter>>("/bots/" + m_id + "/votes?page=" + std::to_string(page), callback, [](const auto& j) {
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