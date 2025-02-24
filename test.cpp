#include <topgg/topgg.h>
#include <dpp/dpp.h>

#include <semaphore>
#include <iostream>
#include <cstdlib>
#include <chrono>

#define ACQUIRE_TEST_THREAD()         \
  g_sem.acquire();                    \
  if (g_exit_code != 0) {             \
    goto TEST_END;                    \
  }                                   \
  std::this_thread::sleep_for(1s)

#define TEST_RESULT_CALLBACK()                           \
  [](const auto& raw_result) {                           \
    try {                                                \
      const auto _result{raw_result.get()};              \
      std::cout << "ok" << std::endl;                    \
    } catch (const std::exception& exc) {                \
      g_exit_code = 1;                                   \
      std::cerr << "error: " << exc.what() << std::endl; \
    }                                                    \
    g_sem.release();                                     \
  }

using namespace std::chrono_literals;

static std::binary_semaphore g_sem{0};
static int g_exit_code{};

int main() {
  const auto discord_token{std::getenv("BOT_TOKEN")};
  const auto topgg_token{std::getenv("TOPGG_TOKEN")};

  if (discord_token == nullptr) {
    std::cerr << "error: missing BOT_TOKEN environment variable" << std::endl;
    return 1;
  } else if (topgg_token == nullptr) {
    std::cerr << "error: missing TOPGG_TOKEN environment variable" << std::endl;
    return 1;
  }

  dpp::cluster bot{discord_token};
  topgg::client topgg_client{bot, topgg_token};

  std::cout << "starting bot ";

  bot.start(dpp::start_type::st_return);

  std::cout << "ok\nget_bot ";

  topgg_client.get_bot(264811613708746752, TEST_RESULT_CALLBACK());
  
  ACQUIRE_TEST_THREAD();
  std::cout << "get_bots ";

  topgg_client
    .get_bots()
    .limit(250)
    .skip(50)
    .username("shiro")
    .sort_by_monthly_votes()
    .finish(TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();
  std::cout << "has_voted ";
  
  topgg_client.has_voted(661200758510977084, TEST_RESULT_CALLBACK());
  
  ACQUIRE_TEST_THREAD();
  std::cout << "post_server_count ";

  topgg_client.post_server_count([](const auto success) {
    if (success) {
      std::cout << "ok" << std::endl;
    } else {
      g_exit_code = 1;
      std::cerr << "error" << std::endl;
    }

    g_sem.release();
  });
  
  ACQUIRE_TEST_THREAD();
  std::cout << "get_server_count ";

  topgg_client.get_server_count(TEST_RESULT_CALLBACK());
  
  ACQUIRE_TEST_THREAD();
  std::cout << "get_voters ";

  topgg_client.get_voters(TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();
  std::cout << "is_weekend ";

  topgg_client.is_weekend(TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();

TEST_END:
  return g_exit_code;
}
