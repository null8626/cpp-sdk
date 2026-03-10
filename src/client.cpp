#include <topgg/topgg.h>

using topgg::client;

client::client(dpp::cluster& cluster, const std::string& token): m_token(token), m_cluster(cluster) {
  m_headers.insert(std::pair("Authorization", "Bearer " + token));
  m_headers.insert(std::pair("Connection", "close"));
  m_headers.insert(std::pair("Content-Type", "application/json"));
  m_headers.insert(std::pair("User-Agent", "topgg (https://github.com/top-gg-community/cpp-sdk) D++"));
}