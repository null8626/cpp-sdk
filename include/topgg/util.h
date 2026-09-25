#pragma once

#ifdef _WIN32
#include <windows.h>
#endif


namespace topgg {
  class http_backend;
  class http_frontend;

  class waker {
#ifdef _WIN32
    HANDLE m_semaphore{nullptr};
#endif

    waker();

    void notify();

    void wait();

    ~waker();

    friend class http_frontend;
    friend class http_backend;
  };

#ifdef _WIN32
  class wsastartup_guard {
    void init();

    ~wsastartup_guard();

    friend class http_backend;
  };
#endif
};