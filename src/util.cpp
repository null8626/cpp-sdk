#include <openssl/rand.h>
#include <topgg/topgg.h>

#include <sstream>

topgg::platform topgg::_from_platform_string(const std::string& platform) {
  if (platform == "discord") {
    return topgg::platform::p_discord;
  } else if (platform == "roblox") {
    return topgg::platform::p_roblox;
  }

  return topgg::platform::p_unknown;
}

topgg::project_type topgg::_from_project_type_string(const std::string& type) {
  if (type == "bot") {
    return topgg::project_type::pt_bot;
  } else if (type == "server") {
    return topgg::project_type::pt_server;
  } else if (type == "game") {
    return topgg::project_type::pt_game;
  }

  return topgg::project_type::pt_unknown;
}

time_t topgg::_from_time_string(const std::string& timestamp) {
  std::tm tm{};
  std::istringstream ss{timestamp};

  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

  if (ss.fail()) {
    throw topgg::exception{"Unable to parse ISO timestamp"};
  }

#ifdef _WIN32
  return _mkgmtime(&tm);
#else
  return timegm(&tm);
#endif
}

std::string topgg::_to_time_string(const time_t timestamp) {
  std::ostringstream ss{};

  ss << std::put_time(std::gmtime(&timestamp), "%FT%TZ");

  return ss.str();
}

std::string topgg::_url_encode(const std::string& text) {
  std::ostringstream ss{};

  ss << std::hex << std::uppercase;

  for (const auto c : text) {
    if (std::isalnum(static_cast<uint8_t>(c)) || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      ss << c;
    } else if (c == ' ') {
      ss << '+';
    } else {
      ss << '%' << std::setw(2) << std::setfill('0')
         << static_cast<int>(static_cast<uint8_t>(c));
    }
  }

  return ss.str();
}

std::string topgg::_random_string() {
  std::vector<uint8_t> buf(64);

  if (RAND_bytes(buf.data(), 64) != 1) {
    throw topgg::exception{"Unable to generate randomized string"};
  }

  std::stringstream ss{};

  ss << std::hex << std::setfill('0');

  for (const auto c : buf) {
    ss << std::setw(2) << static_cast<int>(c);
  }

  return ss.str();
}

const char* topgg::_platform_to_string(const topgg::platform& platform) {
  switch (platform) {
    case topgg::platform::p_discord:
      return "discord";
    case topgg::platform::p_roblox:
      return "roblox";
    default:
      return "unknown";
  }
}

const char* topgg::_project_type_to_string(const topgg::project_type& type) {
  switch (type) {
    case topgg::project_type::pt_bot:
      return "bot";
    case topgg::project_type::pt_server:
      return "server";
    case topgg::project_type::pt_game:
      return "game";
    default:
      return "unknown";
  }
}