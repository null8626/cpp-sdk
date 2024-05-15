#pragma once

#include <dpp/dpp.h>
#include <topgg/dpp.h>

#include <unordered_set>
#include <chrono>
#include <memory>
#include <mutex>

#if __cplusplus < 202002L
#include <condition_variable>
#else
#include <semaphore>
#endif

namespace topgg {
  namespace autoposter {
#if __cplusplus < 202002L
    class cached;
    
    class semaphore {
      std::mutex m_mutex;
      std::condition_variable m_condition;
      size_t m_count;

      inline semaphore(const size_t count): m_count(count) {}
      
      void release();
      void acquire();
      
      friend class cached;
    public:
      semaphore() = delete;
    };
#else
    using semaphore = std::binary_semaphore;
#endif
    
    class base;

    // such a menacing name
    class killable_waiter {
      std::mutex m_mutex;
      std::condition_variable m_condition;
      size_t m_killed;

      inline killable_waiter(): m_killed(false) {}
      
      template<class R, class P>
      bool wait(const std::chrono::duration<R, P>& delay);
      void kill();
      
      friend class base;
    };
    
    class base {
      killable_waiter m_waiter;
      std::thread m_thread;
      
      virtual inline void before_fetch() {}
      virtual inline void after_fetch() {}
      virtual ::topgg::stats get_stats(dpp::cluster* bot) = 0;
      
    protected:
      std::shared_ptr<dpp::cluster> m_cluster;
      
      template<class R, class P>
      base(std::shared_ptr<dpp::cluster>& cluster, const std::string& token, const std::chrono::duration<R, P>& delay);
    public:
      virtual void stop();
      
      ~base();
    };
    
    class cached: public base {
      std::mutex m_mutex;
      semaphore m_semaphore;
      std::unordered_set<dpp::snowflake> m_guilds;
      
      inline void before_fetch() override {
        m_semaphore.acquire();
        m_mutex.lock();
      }
      
      inline void after_fetch() override {
        m_mutex.unlock();
      }
      
      inline ::topgg::stats get_stats(dpp::cluster* _) override {
        return ::topgg::stats{m_guilds.size()};
      }
      
    public:
      template<class R, class P>
      cached(std::shared_ptr<dpp::cluster>& cluster, const std::string& token, const std::chrono::duration<R, P>& delay);
    
      void stop();
    };
    
    class custom: public base {
      std::function<::topgg::stats(dpp::cluster*)> m_callback;
      
      inline ::topgg::stats get_stats(dpp::cluster* bot) override {
        return m_callback(bot);
      }
      
    public:
      template<class R, class P>
      inline custom(std::shared_ptr<dpp::cluster>& cluster, const std::string& token, const std::chrono::duration<R, P>& delay, std::function<::topgg::stats(dpp::cluster*)> callback): base(cluster, token, delay), m_callback(callback) {}
    };
  };
};