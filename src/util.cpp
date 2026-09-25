#include <topgg/exception.h>
#include <topgg/util.h>

#ifdef _WIN32
#include <winsock2.h>


static thread_local size_t g_wsastartup_counter = 0;
#endif

topgg::waker::waker() {
#ifdef _WIN32
  if ((m_semaphore = CreateSemaphoreA(nullptr, 0, 1, nullptr)) == nullptr) {
    throw topgg::exception::system("Unable to create semaphore");
  }
#endif
}

void topgg::waker::notify() {
#ifdef _WIN32
  if (ReleaseSemaphore(m_semaphore, 1, nullptr) == 0 && GetLastError() != ERROR_TOO_MANY_POSTS) {
    throw topgg::exception::system("Unable to notify semaphore");
  }
#endif
}

void topgg::waker::wait() {
#ifdef _WIN32
  if (WaitForSingleObject(m_semaphore, INFINITE) == WAIT_FAILED) {
    throw topgg::exception::system("Unable to wait for semaphore");
  }
#endif
}

topgg::waker::~waker() {
#ifdef _WIN32
  if (m_semaphore != nullptr) {
    CloseHandle(m_semaphore);
  }
#endif
}

#ifdef _WIN32
void topgg::wsastartup_guard::init() {
  if (g_wsastartup_counter == 0) {
    WSADATA wsa_data{};

    if (const auto status{WSAStartup(MAKEWORD(2, 2), &wsa_data)}; status != 0) {
      throw topgg::exception::system("Unable to call WSAStartup");
    }

    g_wsastartup_counter++;
  }
}

topgg::wsastartup_guard::~wsastartup_guard() {
  if (g_wsastartup_counter > 0 && g_wsastartup_counter-- == 1) {
    WSACleanup();
  }
}
#endif