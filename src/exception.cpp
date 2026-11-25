#include <openssl/err.h>
#include <topgg/topgg.h>

using topgg::exception;
using topgg::request_exception;

void exception::assert_curle(const CURLcode code, char* details) {
  if (code != CURLE_OK) {
    const auto brief_message{curl_easy_strerror(code)};
    auto full_message{std::string{"curl easy error: "} +
                      std::string{brief_message}};

    if (details[0] != 0) {
      full_message.append(" (");
      full_message.append(details);
      full_message.append(")");
    }

    throw exception{full_message};
  }
}

void exception::assert_curlm(const CURLMcode code) {
  if (code != CURLM_OK) {
    throw exception{std::string{"curl multi error: "} + std::to_string(code)};
  }
}

exception exception::ssl(const std::string& message) {
  auto full_message{message + " ("};
  unsigned long error{};

  while ((error = ERR_get_error()) != 0) {
    full_message += ERR_error_string(error, nullptr);
    full_message += ", ";
  }

  if (full_message.size() >= 2 &&
      full_message.compare(full_message.size() - 2, 2, ", ") == 0) {
    full_message.erase(full_message.size() - 2);
  }

  return full_message;
}

request_exception request_exception::parse(
    const topgg::http_response& response) {
  auto message{std::string{"Got "} + std::to_string(response.status)};

  std::optional<std::string> title{std::nullopt};
  std::optional<std::string> detail{std::nullopt};
  std::optional<std::string> code{std::nullopt};

  nlohmann::json j{};

  if (!response.body.empty()) {
    try {
      j = nlohmann::json::parse(response.body);
    } catch (const nlohmann::json::exception&) {
    }
  }

  const char* key{nullptr};

  if (j.contains("code")) {
    key = "code";
  } else if (j.contains("error")) {
    key = "error";
  }

  if (key != nullptr) {
    const auto code_{j[key].template get<std::string>()};

    message.append(" (");
    message.append(code_);
    message.append(")");

    code = std::optional{code_};
  }

  if (j.contains("title") && j.contains("detail")) {
    const auto title_{j["title"].template get<std::string>()};
    const auto detail_{j["detail"].template get<std::string>()};

    message = title_ + " (" + detail_ + ")";

    title = std::optional{title_};
    detail = std::optional{detail_};
  }

  request_exception error{response.status, message};

  error.type = j.contains("type")
                   ? j["type"].template get<std::optional<std::string>>()
                   : std::nullopt;
  error.title = title;
  error.detail = detail;
  error.code = code;

  return error;
}