#include "typesafe.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void rejects(Function function, const char* message) {
  bool threw = false;
  try {
    function();
  } catch (const std::exception&) {
    threw = true;
  }
  require(threw, message);
}

}  // namespace

int main() {
  try {
    using namespace ailike_plugin;
    const std::string text = std::string("quotation \" \\ \n") + '\0' + " שלום";
    const auto request = nlohmann::json::parse(make_request(text, "a film about love", "jev-test"));
    require(request.at("state").at("value") == text, "text changed during JSON encoding");
    require(request.at("model") == "jev-test", "model not preserved");
    require(request.at("questions").at("matches").at("type") == "noul", "wrong primitive");
    require(request.at("questions").at("matches").at("instructions").at("condition") ==
                "a film about love", "condition not preserved");
    for (double probability : {0.0, 0.5, 1.0}) {
      const auto body = nlohmann::json{{"answers", {{"matches", {{"type", "noul"}, {"noul", probability}}}}}}.dump();
      require(parse_response(body) == probability, "valid probability rejected");
    }
    for (const auto* response : {
             "not json", "{}", R"({"answers":{"matches":{"type":"score","noul":0.5}}})",
             R"({"answers":{"matches":{"type":"noul","noul":"0.5"}}})",
             R"({"answers":{"matches":{"type":"noul","noul":true}}})",
             R"({"answers":{"matches":{"type":"noul","noul":null}}})",
             R"({"answers":{"matches":{"type":"noul","noul":-0.1}}})",
             R"({"answers":{"matches":{"type":"noul","noul":1.1}}})",
             R"({"answers":{"matches":{"type":"noul","noul":1e999}}})"}) {
      rejects([&] { parse_response(response); }, "invalid response accepted");
    }
    rejects([] { make_request("ok", " \t\r\n", "jev-test"); }, "empty prompt accepted");
    rejects([] { make_request(std::string(kMaxValueBytes + 1, 'x'), "ok", "jev-test"); }, "oversized text accepted");
    rejects([] { make_request("ok", std::string(kMaxPromptBytes + 1, 'x'), "jev-test"); }, "oversized prompt accepted");
    rejects([] { make_request(std::string(1, '\xff'), "ok", "jev-test"); }, "invalid UTF-8 accepted");
    std::cout << "TypeSafe contract tests passed\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
