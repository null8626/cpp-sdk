#include <topgg/topgg.h>
#include <dpp/dpp.h>

#include <semaphore>
#include <iostream>
#include <cstdlib>
#include <chrono>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

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
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

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
  topgg::v1client client{bot, topgg_token};

  std::cout << "Starting bot... ";

  bot.start(dpp::start_type::st_return);

  std::cout << "ok\nget_bot ";

  client.get_bot(264811613708746752, TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();
  std::cout << "get_bots ";

  client
    .get_bots()
    .limit(250)
    .skip(50)
    .sort_by_monthly_votes()
    .send(TEST_RESULT_CALLBACK());
  
  ACQUIRE_TEST_THREAD();
  std::cout << "has_voted ";

  client.has_voted(661200758510977084, TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();
  std::cout << "post_stats ";

  client.post_stats([](const auto success) {
    if (success) {
      std::cout << "ok" << std::endl;
    } else {
      g_exit_code = 1;
      std::cerr << "error" << std::endl;
    }

    g_sem.release();
  });
  
  ACQUIRE_TEST_THREAD();
  std::cout << "get_stats ";

  client.get_stats(TEST_RESULT_CALLBACK());
  
  ACQUIRE_TEST_THREAD();
  std::cout << "get_voters ";

  client.get_voters(TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();
  std::cout << "is_weekend ";

  client.is_weekend(TEST_RESULT_CALLBACK());

  ACQUIRE_TEST_THREAD();
  std::cout << "post_commands ";

  client.post_commands([](const auto result) {
    if (result) {
      std::cout << "ok" << std::endl;
    } else {
      g_exit_code = 1;
      std::cerr << "error" << std::endl;
    }

    g_sem.release();
  });

  ACQUIRE_TEST_THREAD();
  std::cout << "get_vote ";

  client.get_vote(661200758510977084, TOPGG_USER_SOURCE_DISCORD, TEST_RESULT_CALLBACK());

  std::cout << "Done." << std::endl;

TEST_END:
  return g_exit_code;
}
