#pragma once

#include <topgg/result.h>
#include <topgg/http.h>
#include <functional>
#include <string>


namespace topgg {
  template<class T>
  using callback = std::function<void(const result<T>&)>;
  using empty_callback = std::function<void(const empty_result&)>;
  template<class T>
  using paginated_callback = std::function<void(const paginated_result<T>&)>;

  class client: public http_frontend {
    void internal_get_projects(const std::string& path, const paginated_callback<partial_project>& callback, const bool defer);

  public:
    inline client(const std::string& token): http_frontend(token) {}

    inline void get_projects(const paginated_callback<partial_project>& callback, const bool defer = false) {
      internal_get_projects("/projects", callback, defer);
    }

    void get_projects(const paginated_result<partial_project>& cursor, const paginated_callback<partial_project>& callback, const bool defer = false);
  };
};