#ifndef NGHTTP2_NO_SSIZE_T
#define NGHTTP2_NO_SSIZE_T
#endif

#include <topgg/exception.h>
#include <nghttp2/nghttp2.h>
#include <openssl/err.h>
#include <uv.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif


topgg::exception topgg::exception::ssl(const char* message) {
  topgg::exception exc{message};

  unsigned long error{};

  while ((error = ERR_get_error()) != 0) {
    exc.cause += ERR_error_string(error, nullptr);
    exc.cause += "\n";
  }

  return exc;
}

topgg::exception topgg::exception::uv(const char* message, const int status) {
  topgg::exception exc{message};

  exc.cause = const_cast<char*>(uv_err_name(status));

  return exc;
}

topgg::exception topgg::exception::system(const char* message) {
  topgg::exception exc{message};

#ifdef _WIN32
  auto error{GetLastError()};

  if (error == 0) {
    error = WSAGetLastError();

    WSASetLastError(0);
  } else {
    SetLastError(0);
  }

  if (error != 0) {
    LPSTR cause_buf{nullptr};

    const auto size{FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&cause_buf), 0, nullptr)};

    if (size > 0) {
      exc.cause = std::string{cause_buf, size};

      LocalFree(cause_buf);
    }
  }
#endif

  return exc;
}

topgg::exception topgg::exception::nghttp2(const char* message, const int status) {
  topgg::exception exc{message};

  exc.cause = const_cast<char*>(nghttp2_strerror(status));

  return exc;
}

topgg::http_exception::http_exception(const std::pair<uint16_t, std::string_view>& response_pair): topgg::exception("Got an invalid HTTP status code from Top.gg"), status(response_pair.first) {
  try {
    const auto j{nlohmann::json::parse(response_pair.second)};

    cause = j["title"].template get<std::string>();
    type = j["type"].template get<std::string>();
    detail = j["detail"].template get<std::string>();
  } catch (const nlohmann::json::parse_error&) {}
}