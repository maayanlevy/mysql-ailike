#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace ailike_plugin {

constexpr std::size_t kMaxValueBytes = 32768;
constexpr std::size_t kMaxPromptBytes = 8192;
constexpr std::size_t kMaxResponseBytes = 1024 * 1024;

struct Settings {
  std::string api_key;
  std::string api_url = "https://api.typesafe.ai/v1/systemone";
  std::string model = "jev-1.13.0";
  std::size_t max_requests = 1000;
  long timeout_ms = 10000;
  long connect_timeout_ms = 3000;

  static Settings from_environment();
};

// These pure functions also define and test the wire contract.
std::string make_request(std::string_view value, std::string_view prompt,
                         std::string_view model);
std::string make_request(std::string_view left, std::string_view right,
                         std::string_view prompt, std::string_view model);
double parse_response(std::string_view body);

// One client belongs to one UDF expression, never to global server state.
// It reuses its HTTP connection and caches a bounded set of exact requests.
class Client {
 public:
  explicit Client(Settings settings);
  ~Client();
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  double score(std::string_view value, std::string_view prompt);
  double score(std::string_view left, std::string_view right,
               std::string_view prompt);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ailike_plugin
