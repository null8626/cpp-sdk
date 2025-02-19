#include <topgg/topgg.h>

using topgg::account;
using topgg::bot;
using topgg::bot_query;
using topgg::stats;
using topgg::user;
using topgg::user_socials;

#ifdef _WIN32
#include <sstream>
#include <iomanip>

static void strptime(const char* s, const char* f, tm* t) {
  std::istringstream input{s};
  input.imbue(std::locale{setlocale(LC_ALL, nullptr)});
  input >> std::get_time(t, f);
}

#ifdef _MSC_VER
#pragma warning(disable: 4101)
#endif
#endif

#define SERIALIZE_PRIVATE_OPTIONAL(j, name) \
  if (m_##name.has_value()) {               \
    j[#name] = m_##name.value();            \
  }

#define DESERIALIZE(j, name, type) \
  name = j[#name].template get<type>()

#define DESERIALIZE_ALIAS(j, name, prop, type) \
  prop = j[#name].template get<type>()

#define IGNORE_EXCEPTION(scope) \
  try scope catch (TOPGG_UNUSED const std::exception&) {}

#define DESERIALIZE_VECTOR(j, name, type)                  \
  IGNORE_EXCEPTION({                                       \
    name = j[#name].template get<std::vector<type>>();     \
  })

#define DESERIALIZE_VECTOR_ALIAS(j, name, prop, type)      \
  IGNORE_EXCEPTION({                                       \
    prop = j[#name].template get<std::vector<type>>();     \
  })

#define DESERIALIZE_OPTIONAL(j, name, type)   \
  IGNORE_EXCEPTION({                          \
    name = j[#name].template get<type>();     \
  })

#define DESERIALIZE_PRIVATE_OPTIONAL(j, name, type)   \
  IGNORE_EXCEPTION({                                  \
    m_##name = j[#name].template get<type>();         \
  })

#define DESERIALIZE_OPTIONAL_ALIAS(j, name, prop) \
  IGNORE_EXCEPTION({                              \
    prop = j[#name].template get<type>();         \
  })

#define DESERIALIZE_OPTIONAL_STRING(j, name)                      \
  IGNORE_EXCEPTION({                                              \
    const auto value{j[#name].template get<std::string>()};       \
                                                                  \
    if (value.size() > 0) {                                       \
      name = std::optional{value};                                \
    }                                                             \
  })

#define DESERIALIZE_OPTIONAL_STRING_ALIAS(j, name, prop)          \
  IGNORE_EXCEPTION({                                              \
    const auto value{j[#name].template get<std::string>()};       \
                                                                  \
    if (value.size() > 0) {                                       \
      prop = std::optional{value};                                \
    }                                                             \
  })

#define ADD_QUERY(key, value) \
  m_query.append(key);        \
  m_query.push_back('=');     \
  m_query.append(value);      \
  m_query.push_back('&')

#define ADD_SEARCH(key, value) \
  m_search.append(key);        \
  m_search.append("%3A%20");   \
  m_search.append(value);      \
  m_search.append("%20")

account::account(const dpp::json& j) {
  id = dpp::snowflake{j["id"].template get<std::string>()};

  DESERIALIZE(j, username, std::string);

  try {
    const auto hash{j["avatar"].template get<std::string>()};
    const char* ext{hash.rfind("a_", 0) == 0 ? "gif" : "png"};

    avatar = "https://cdn.discordapp.com/avatars/" + std::to_string(id) + "/" + hash + "." + ext + "?size=1024";
  } catch (TOPGG_UNUSED const std::exception&) {
    avatar = "https://cdn.discordapp.com/embed/avatars/" + std::to_string((id >> 22) % 6) + ".png";
  }

  created_at = static_cast<time_t>(((id >> 22) / 1000) + 1420070400);
}

bot::bot(const dpp::json& j)
  : account(j), url("https://top.gg/bot/") {
  // TODO: remove this soon
  discriminator = "0";
  
  DESERIALIZE(j, prefix, std::string);
  DESERIALIZE_ALIAS(j, shortdesc, short_description, std::string);
  DESERIALIZE_OPTIONAL_STRING_ALIAS(j, longdesc, long_description);
  DESERIALIZE_VECTOR(j, tags, std::string);
  DESERIALIZE_OPTIONAL_STRING(j, website);
  DESERIALIZE_OPTIONAL_STRING(j, github);

  IGNORE_EXCEPTION({
    const auto j_owners{j["owners"].template get<std::vector<std::string>>()};

    owners.reserve(j_owners.size());

    for (const auto& owner: j_owners) {
      owners.push_back(dpp::snowflake{owner});
    }
  });

  DESERIALIZE_OPTIONAL_STRING_ALIAS(j, bannerUrl, banner);

  const auto j_approved_at{j["date"].template get<std::string>()};
  tm approved_at_tm;

  strptime(j_approved_at.data(), "%Y-%m-%dT%H:%M:%S", &approved_at_tm);
  approved_at = mktime(&approved_at_tm);

  // TODO: remove this soon
  is_certified = false;

  DESERIALIZE_ALIAS(j, points, votes, size_t);
  DESERIALIZE_ALIAS(j, monthlyPoints, monthly_votes, size_t);

  try {
    DESERIALIZE(j, invite, std::string);
  } catch (TOPGG_UNUSED const std::exception&) {
    invite = "https://discord.com/oauth2/authorize?scope=bot&client_id=" + std::to_string(id);
  }

  IGNORE_EXCEPTION({
    const auto j_support{j["support"].template get<std::string>()};

    if (j_support.size() > 0) {
      support = std::optional{"https://discord.com/invite/" + j_support};
    }
  });

  // TODO: remove this soon
  shard_count = 0;

  try {
    url.append(j["vanity"].template get<std::string>());
  } catch (TOPGG_UNUSED const std::exception&) {
    url.append(std::to_string(id));
  }
}

static std::string querystring(const std::string& value) {
  static constexpr char hex[] = "0123456789abcdef";
  std::string output{};

  output.reserve(value.length());

  for (size_t i{}; i < value.length(); i++) {
    const auto c{value[i]};

    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')) {
      output.push_back(c);
    } else {
      output.push_back('%');
      output.push_back(hex[(c >> 4) & 0x0f]);
      output.push_back(hex[c & 0x0f]);
    }
  }

  return output;
}

void bot_query::add_query(const char* key, const uint16_t value, const uint16_t max) {
  ADD_QUERY(key, std::to_string(std::min(value, max)));
}

void bot_query::add_query(const char* key, const char* value) {
  ADD_QUERY(key, value); // querystring() not needed here
}

void bot_query::add_search(const char* key, const std::string& value) {
  ADD_SEARCH(key, querystring(value));
}

void bot_query::add_search(const char* key, const size_t value) {
  ADD_SEARCH(key, std::to_string(value));
}

void bot_query::finish(const topgg::get_bots_completion_t& callback) {
  if (m_sort != nullptr) {
    add_query("sort", m_sort);
  }

  if (!m_search.empty()) {
    add_query("search", m_search.c_str());
  }

  m_query.pop_back();

  m_client->basic_request<std::vector<topgg::bot>>(m_query, callback, [](const auto& j) {
    std::vector<topgg::bot> bots{};

    for (const auto& part: j["results"].template get<std::vector<dpp::json>>()) {
      bots.push_back(topgg::bot{part});
    }

    return bots;
  });
}

#ifdef DPP_CORO
dpp::async<std::vector<topgg::bot>> bot_query::co_finish() {
  return dpp::async<std::vector<topgg::bot>>{ [this] <typename C> (C&& cc) { return finish(std::forward<C>(cc)); }};
}
#endif

stats::stats(const dpp::json& j) {
  DESERIALIZE_PRIVATE_OPTIONAL(j, server_count, size_t);
}

stats::stats(dpp::cluster& bot) {
  size_t servers{};
  
  for (auto& s: bot.get_shards()) {
    servers += s.second->get_guild_count();
  }
  
  m_server_count = std::optional{servers};
}

// TODO: remove this soon
stats::stats(const std::vector<size_t>& shards, const TOPGG_UNUSED size_t shard_index)
  : m_server_count(std::optional{std::reduce(shards.begin(), shards.end())}) {}

std::string stats::to_json() const {
  dpp::json j{};

  SERIALIZE_PRIVATE_OPTIONAL(j, server_count);

  return j.dump();
}

std::vector<size_t> stats::shards() const noexcept {
  return std::vector<size_t>{};
}

size_t stats::shard_count() const noexcept {
  return 0;
}

std::optional<size_t> stats::server_count() const noexcept {
  return m_server_count;
}

user_socials::user_socials(const dpp::json& j) {
  DESERIALIZE_OPTIONAL_STRING(j, github);
  DESERIALIZE_OPTIONAL_STRING(j, instagram);
  DESERIALIZE_OPTIONAL_STRING(j, reddit);
  DESERIALIZE_OPTIONAL_STRING(j, twitter);
  DESERIALIZE_OPTIONAL_STRING(j, youtube);
}

user::user(const dpp::json& j)
  : account(j) {
  DESERIALIZE_OPTIONAL_STRING(j, bio);
  DESERIALIZE_OPTIONAL_STRING(j, banner);

  if (j.contains("socials")) {
    socials = std::optional{user_socials{j["socials"].template get<dpp::json>()}};
  }

  // TODO: remove this soon
  is_certified_dev = false;

  DESERIALIZE_ALIAS(j, supporter, is_supporter, bool);
  DESERIALIZE_ALIAS(j, mod, is_moderator, bool);
  DESERIALIZE_ALIAS(j, webMod, is_web_moderator, bool);
  DESERIALIZE_ALIAS(j, admin, is_admin, bool);
}
