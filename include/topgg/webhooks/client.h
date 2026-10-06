#pragma once

#include <openssl/params.h>
#include <openssl/evp.h>
#include <topgg/topgg.h>
#include <string>
#include <mutex>


namespace topgg::webhooks {
  payload parse(const std::string& body);

  class verifier {
    std::mutex m_mutex{};
    EVP_MAC* m_mac{};
    EVP_MAC_CTX* m_mac_context{};
    OSSL_PARAM m_params[2];

  public:
    verifier();

    bool verify(const std::string& secret, const std::string& signature, const std::string& body);

    ~verifier();
  };
};