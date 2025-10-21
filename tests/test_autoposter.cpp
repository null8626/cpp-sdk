#include <topgg/topgg.h>
#include <dpp/dpp.h>

#include <semaphore>
#include <iostream>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

static std::binary_semaphore g_sem{0};
static int g_exit_code{};
static size_t g_counter{};

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
  topgg::client client{bot, topgg_token};

  std::cout << "Starting bot... ";

  bot.start(dpp::start_type::st_return);

  std::cout << "ok\n";

  client.start_autoposter([](const auto result) {
    if (result) {
      std::cout << "Successfully posted to the API!" << std::endl;
      
      if (g_counter++ == 3) {
        g_sem.release();
      }
    } else {
      std::cerr << "Failed." << std::endl;
      
      g_exit_code = 1;
      g_sem.release();
    }
  }, 2);
  
  g_sem.acquire();
  client.stop_autoposter();

  std::cout << "Done." << std::endl;

  return g_exit_code;
}
