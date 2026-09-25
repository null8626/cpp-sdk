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

  exc.m_cause = ERR_error_string(ERR_get_error(), nullptr);

  return exc;
}

topgg::exception topgg::exception::uv(const char* message, const int status) {
  topgg::exception exc{message};

  exc.m_cause = const_cast<char*>(uv_err_name(status));

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
      exc.m_cause = std::string{cause_buf, size};

      LocalFree(cause_buf);
    }
  }
#endif

  return exc;
}

topgg::exception topgg::exception::nghttp2(const char* message, const int status) {
  topgg::exception exc{message};
  
  exc.m_cause = const_cast<char*>(nghttp2_strerror(status));

  return exc;
}