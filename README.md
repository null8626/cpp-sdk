# Top.gg C++ SDK

The community-maintained C++17 library for Top.gg.

## Building from source

First, clone the git repository like so:

```sh
$ git clone https://github.com/Top-gg-Community/cpp-sdk --depth 1
```

**NOTE:** To enable C++20 coroutine methods, add `-DENABLE_CORO=ON`!

### Linux (Debian-like)

Install D++:

```sh
$ wget -O dpp.deb https://dl.dpp.dev/latest
$ dpkg -i dpp.deb
```

Build `topgg`:

```sh
$ cmake -B build .
$ cmake --build build --config Release
```

### Linux (CentOS-like)

Install D++:

```sh
$ yum install wget
$ wget -O dpp.rpm https://dl.dpp.dev/latest/linux-x64/rpm
$ yum localinstall dpp.rpm
```

Build `topgg`:

```sh
$ cmake -B build .
$ cmake --build build --config Release
```

### macOS

Install D++:

```sh
$ brew install libdpp
$ brew link libdpp
```

Build `topgg`:

```sh
$ cmake -B build .
$ cmake --build build --config Release
```

### Windows

```bat
> cmake -B build .
> cmake --build build --config Release
```

## Setting up

```cpp
#include <topgg/topgg.h>
#include <dpp/dpp.h>

#include <iostream>
#include <utility>

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
  topgg::client client{bot, topgg_token};

  return 0;
}
```

## Usage

### Getting a bot

#### With C++17 callbacks

```cpp
client.get_bot(264811613708746752, [](const auto& result) {
  try {
    const auto topgg_bot = result.get();

    std::cout << topgg_bot.username << std::endl;
  } catch (const std::exception& exc) {
    std::cerr << "error: " << exc.what() << std::endl;
  }
});
```

#### With C++20 coroutines

```cpp
try {
  const auto topgg_bot = co_await client.co_get_bot(264811613708746752);

  std::cout << topgg_bot.username << std::endl;
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}
```

### Getting several bots

#### With C++17 callbacks

```cpp
client
  .get_bots()
  .limit(250)
  .skip(50)
  .sort_by_monthly_votes()
  .send([](const auto& result) {
    try {
      const auto bots = result.get();

      for (const auto& bot: bots) {
        std::cout << bot.username << std::endl;
      }
    } catch (const std::exception& exc) {
      std::cerr << "error: " << exc.what() << std::endl;
    }
  });
```

#### With C++20 coroutines

```cpp
try {
  const auto bots = co_await client
    .get_bots()
    .limit(250)
    .skip(50)
    .sort_by_monthly_votes()
    .send();

  for (const auto& bot: bots) {
    std::cout << topgg_bot.username << std::endl;
  }
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}
```

### Getting your bot's voters

#### With C++17 callbacks

```cpp
// First page
client.get_voters([](const auto& result) {
  try {
    auto voters = result.get();

    for (auto& voter: voters) {
      std::cout << voter.username << std::endl;
    }
  } catch (const std::exception& exc) {
    std::cerr << "error: " << exc.what() << std::endl;
  }
});

// Subsequent pages
client.get_voters(2, [](const auto& result) {
  try {
    auto voters = result.get();

    for (auto& voter: voters) {
      std::cout << voter.username << std::endl;
    }
  } catch (const std::exception& exc) {
    std::cerr << "error: " << exc.what() << std::endl;
  }
});
```

#### With C++20 coroutines

```cpp
// First page
try {
  const auto voters = co_await client.co_get_voters();

  for (const auto& voter: voters) {
    std::cout << voter.username << std::endl;
  }
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}

// Subsequent pages
try {
  const auto voters = co_await client.co_get_voters(2);

  for (const auto& voter: voters) {
    std::cout << voter.username << std::endl;
  }
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}
```

### Check if a user has voted for your bot

#### With C++17 callbacks

```cpp
client.has_voted(661200758510977084, [](const auto& result) {
  try {
    if (result.get()) {
      std::cout << "Checks out." << std::endl;
    }
  } catch (const std::exception& exc) {
    std::cerr << "error: " << exc.what() << std::endl;
  }
});
```

#### With C++20 coroutines

```cpp
try {
  const auto voted = co_await client.co_has_voted(661200758510977084);

  if (voted) {
    std::cout << "Checks out." << std::endl;
  }
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}
```

### Getting your bot's server count

#### With C++17 callbacks

```cpp
client.get_server_count([](const auto& result) {
  try {
    auto server_count = result.get();

    std::cout << server_count.value_or(0) << std::endl;
  } catch (const std::exception& exc) {
    std::cerr << "error: " << exc.what() << std::endl;
  }
});
```

#### With C++20 coroutines

```cpp
try {
  const auto server_count = co_await client.co_get_server_count();

  std::cout << server_count.value_or(0) << std::endl;
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}
```

### Posting your bot's server count

#### With C++17 callbacks

```cpp
client.post_server_count([](const auto success) {
  if (success) {
    std::cout << "Stats posted!" << std::endl;
  }
});
```

#### With C++20 coroutines

```cpp
const auto success = co_await client.co_post_server_count();

if (success) {
  std::cout << "Stats posted!" << std::endl;
}
```

### Automatically posting your bot's server count every few minutes

#### Without a callback

```cpp
client.start_autoposter();
```

#### With a callback

```cpp
client.start_autoposter([](const auto& result) {
  if (result) {
    std::cout << "Successfully posted " << *result << " servers to the API!" << std::endl;
  }
});
```

#### From a custom source

```cpp
class my_autoposter_source: private topgg::autoposter_source {
public:
  virtual size_t get_server_count(dpp::cluster& bot) {
    return ...;
  }
};

client.start_autoposter(reinterpret_cast<topgg::autoposter_source*>(new my_autoposter_source), [](const auto& result) {
  if (result) {
    std::cout << "Successfully posted " << *result << " servers to the API!" << std::endl;
  }
});
```

### Checking if the weekend vote multiplier is active

#### With C++17 callbacks

```cpp
client.is_weekend([](const auto& result) {
  try {
    if (result.get()) {
      std::cout << "The weekend multiplier is active" << std::endl;
    }
  } catch (const std::exception& exc) {
    std::cerr << "error: " << exc.what() << std::endl;
  }
});
```

#### With C++20 coroutines

```cpp
try {
  const auto is_weekend = co_await client.co_is_weekend();

  if (is_weekend) {
    std::cout << "The weekend multiplier is active" << std::endl;
  }
} catch (const std::exception& exc) {
  std::cerr << "error: " << exc.what() << std::endl;
}
```

### Generating widget URLs

#### Large

```cpp
const auto widget_url{topgg::widget::large(TOPGG_WIDGET_DISCORD_BOT, 264811613708746752)};
```

#### Votes

```cpp
const auto widget_url{topgg::widget::votes(TOPGG_WIDGET_DISCORD_BOT, 264811613708746752)};
```

#### Owner

```cpp
const auto widget_url{topgg::widget::owner(TOPGG_WIDGET_DISCORD_BOT, 264811613708746752)};
```

#### Social

```cpp
const auto widget_url{topgg::widget::social(TOPGG_WIDGET_DISCORD_BOT, 264811613708746752)};
```