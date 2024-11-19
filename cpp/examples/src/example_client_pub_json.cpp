#include <foxglove/websocket/websocket_client.hpp>
#include <foxglove/websocket/websocket_notls.hpp>

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <regex>
#include <unordered_map>

constexpr char DEFAULT_URI[] = "ws://localhost:8765";

std::atomic<bool> doAbort = false;

void signal_handler(int) {
  doAbort = true;
}

int main(int argc, char** argv) {
  const auto url = argc > 1 ? argv[1] : DEFAULT_URI;

  foxglove::Client<foxglove::WebSocketNoTls> client;

  const auto openHandler = [&](websocketpp::connection_hdl) {
    std::cout << "Connected to " << std::string(url) << std::endl;

    client.advertise({
      {
        .topic = "client_pub_json",
        .encoding = "json",
        .schemaName = "some_schema",
        .schemaEncoding = "json",
        .schema =
          nlohmann::json{
            {"type", "object"},
            {"properties",
             {
               {"seq", {{"type", "number"}}},
               {"x", {{"type", "number"}}},
               {"y", {{"type", "number"}}},
             }},
          }
            .dump(),
      },
    });
  };
  const auto closeHandler = [&](websocketpp::connection_hdl) {
    std::cout << "Connection closed" << std::endl;
    doAbort = true;
  };

  client.connect(url, openHandler, closeHandler);
  std::signal(SIGINT, signal_handler);

  while (!doAbort) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  client.close();
}