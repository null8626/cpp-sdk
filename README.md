# Top.gg C++ SDK

The community-maintained C++17 library for Top.gg.

## Building from source

First, clone the git repository like so:

```sh
$ git clone https://github.com/Top-gg-Community/cpp-sdk --depth 1
$ cd cpp-sdk
$ git submodule update --init --recursive
```

The C++ SDK provides building options that you can enable or disable by setting the corresponding variables to `ON` or `OFF`. They are as follows:

| Option name                   | Description                                      | Default |
| ----------------------------- | ------------------------------------------------ | ------- |
| `BUILD_SHARED_LIBS`           | Build shared libraries.                          | `ON`    |
| `ENABLE_API`                  | Build primary API support.                       | `ON`    |
| `ENABLE_CPP_HTTPLIB_WEBHOOKS` | Build support for webhooks via `cpp-httplib`.    | `OFF`   |
| `ENABLE_DROGON_WEBHOOKS`      | Build support for webhooks via `drogon`.         | `OFF`   |
| `ENABLE_CORO`                 | Add support for C++20 coroutines.                | `OFF`   |
| `TESTING`                     | Enable this only if you are testing the library. | `OFF`   |

### Main API wrapper

#### Linux (Debian-like)

Install D++:

```sh
$ wget -O dpp.deb https://dl.dpp.dev/latest
$ sudo dpkg -i dpp.deb
```

Build `topgg`:

```sh
$ cmake -B build .
$ cmake --build build --config Release
```

#### Linux (CentOS-like)

Install D++:

```sh
$ sudo yum install wget
$ wget -O dpp.rpm https://dl.dpp.dev/latest/linux-x64/rpm
$ sudo yum localinstall dpp.rpm
```

Build `topgg`:

```sh
$ cmake -B build .
$ cmake --build build --config Release
```

#### macOS

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

#### Windows

Install D++ and build `topgg`:

```bat
> cmake -B build .
> cmake --build build --config Release
```

### Webhooks only

#### cpp-httplib

Install `cpp-httplib` and build `topgg`:

```bat
> cmake -DENABLE_API=OFF -DENABLE_CPP_HTTPLIB_WEBHOOKS=ON -B build .
> cmake --build build --config Release
```

#### Drogon

##### Linux (Debian-like)

Install the C/C++ compiler(s):

```sh
$ sudo apt install git gcc g++ cmake
```

Install Drogon's dependencies:

```sh
$ sudo apt install libjsoncpp-dev uuid-dev zlib1g-dev
```

Install Drogon and build `topgg`:

```sh
$ cmake -DENABLE_API=OFF -DENABLE_DROGON_WEBHOOKS=ON -B build .
$ cmake --build build --config Release
```

##### Linux (Arch-like)

Install the C/C++ compiler(s):

```sh
$ sudo pacman -S git gcc make cmake
```

Install Drogon's dependencies:

```sh
$ sudo pacman -S jsoncpp uuid zlib
```

Install Drogon and build `topgg`:

```sh
$ cmake -DENABLE_API=OFF -DENABLE_DROGON_WEBHOOKS=ON -B build .
$ cmake --build build --config Release
```

##### Linux (CentOS-like)

Install the C/C++ compiler(s):

```sh
$ sudo yum install git gcc gcc-c++
```

Install the latest version of CMake from source if you haven't already:

```sh
$ git clone https://github.com/Kitware/CMake --depth 1
$ cd CMake
$ ./bootstrap && make && make install
$ cd ..
```

Update your system's GCC:

```sh
$ sudo yum install centos-release-scl devtoolset-11
$ scl enable devtoolset-11 bash
```

Install Drogon's dependencies:

```sh
$ git clone https://github.com/open-source-parsers/jsoncpp --depth 1
$ cd jsoncpp
$ mkdir build && cd build
$ cmake .. && make && make install
$ cd ../..
$ sudo yum install libuuid-devel zlib-devel
```

Install Drogon and build `topgg`:

```sh
$ cmake -DENABLE_API=OFF -DENABLE_DROGON_WEBHOOKS=ON -B build .
$ cmake --build build --config Release
```

##### macOS

Install Drogon's dependencies:

```sh
$ brew upgrade
$ brew install jsoncpp ossp-uuid zlib-devel
```

Install Drogon and build `topgg`:

```sh
$ cmake -DENABLE_API=OFF -DENABLE_DROGON_WEBHOOKS=ON -B build .
$ cmake --build build --config Release
```

##### Windows

Install `conan` if you haven't already:

```bat
> pip install conan
```

Install Drogon's dependencies:

```bat
> conan profile detect --force
> conan install . -s compiler="msvc" -s compiler.version=193 -s compiler.cppstd=17 -s build_type=Release --output-folder . --build=missing -g CMakeToolchain
```

Install Drogon and build `topgg`:

```bat
> cmake -DENABLE_API=OFF -DENABLE_DROGON_WEBHOOKS=ON -B build .
> cmake --build build --config Release
```

## Setting up

```cpp
// If you compiled this library statically, uncomment the following:
// #ifndef TOPGG_STATIC
// #define TOPGG_STATIC
// #endif

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
    std::cout << "Successfully posted " << *result << " servers to Top.gg!" << std::endl;
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
    std::cout << "Successfully posted " << *result << " servers to Top.gg!" << std::endl;
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

### Webhooks

#### Being notified whenever someone voted for your bot

##### cpp-httplib

```cpp
// If you compiled this library statically, uncomment the following:
// #ifndef TOPGG_STATIC
// #define TOPGG_STATIC
// #endif

#include <cpp-httplib/cpp-httplib.h>
#include <topgg/webhooks/cpp-httplib.h>

#include <iostream>
#include <string>

template<class T>
using cpp_httplib_webhook = topgg::webhook::cpp_httplib<T>;
using topgg::webhook::vote;

class my_vote_listener: public cpp_httplib_webhook<vote> {
public:
  inline my_vote_listener(const std::string& authorization): cpp_httplib_webhook<vote>(authorization) {}

  void callback(const vote& v) override {
    std::cout << "A user with the ID of " << v.voter_id << " has voted us on Top.gg!" << std::endl;
  }
};

int main() {
  const auto authorization{std::getenv("MY_TOPGG_WEBHOOK_SECRET")};

  if (authorization == nullptr) {
    std::cerr << "error: missing MY_TOPGG_WEBHOOK_SECRET environment variable" << std::endl;
    return 1;
  }

  httplib::Server server{};
  my_vote_listener webhook{authorization};

  server.Post("/votes", webhook.endpoint());
  server.listen("localhost", 8080);
  
  return 0;
}
```

##### Drogon

```cpp
// If you compiled this library statically, uncomment the following:
// #ifndef TOPGG_STATIC
// #define TOPGG_STATIC
// #endif

#include <topgg/webhooks/drogon.h>

#include <iostream>
#include <memory>
#include <string>

template<class T>
using drogon_webhook = topgg::webhook::drogon<T>;
using topgg::webhook::vote;

class my_vote_listener: public ::drogon::HttpSimpleController<my_vote_listener, false>, public drogon_webhook<vote> {
public:
  inline my_vote_listener(const std::string& authorization): drogon_webhook<vote>(authorization) {}

  TOPGG_DROGON_WEBHOOK();

  PATH_LIST_BEGIN
  PATH_ADD("/votes", ::drogon::Post);
  PATH_LIST_END

  void callback(const vote& v) override {
    std::cout << "A user with the ID of " << v.voter_id << " has voted us on Top.gg!" << std::endl;
  }
};

int main() {
  const auto authorization{std::getenv("MY_TOPGG_WEBHOOK_SECRET")};

  if (authorization == nullptr) {
    std::cerr << "error: missing MY_TOPGG_WEBHOOK_SECRET environment variable" << std::endl;
    return 1;
  }

  auto& app{drogon::app()};

  app.registerController(std::make_shared<my_vote_listener>(authorization));
  app.addListener("127.0.0.1", 8080);
  app.run();

  return 0;
}
```