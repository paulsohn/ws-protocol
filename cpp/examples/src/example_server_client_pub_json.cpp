#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/websocket_notls.hpp>
#include <foxglove/websocket/websocket_server.hpp>

#include <atomic>
#include <csignal>
#include <iostream>
#include <unordered_map>

std::atomic<bool> doAbort = false;

void signal_handler(int) {
  std::cout << "Received SIGINT, exiting..." << std::endl;
  doAbort = true;
}

void logCallback(foxglove::WebSocketLogLevel level, char const* msg) {
  switch (level) {
    case foxglove::WebSocketLogLevel::Debug:
      std::cout << "[DEBUG]" << msg << std::endl;
      break;
    case foxglove::WebSocketLogLevel::Info:
      std::cout << "[INFO]" << msg << std::endl;
      break;
    case foxglove::WebSocketLogLevel::Warn:
      std::cout << "[WARN]" << msg << std::endl;
      break;
    case foxglove::WebSocketLogLevel::Error:
      std::cerr << "[ERROR]" << msg << std::endl;
      break;
    case foxglove::WebSocketLogLevel::Critical:
      std::cerr << "[FATAL]" << msg << std::endl;
      break;
  }
}

int main(int argc, char** argv) {
  foxglove::ServerOptions serverOptions;
  serverOptions.capabilities = {"clientPublish"};
  serverOptions.sessionId = std::to_string(std::time(nullptr));
  serverOptions.clientTopicWhitelistPatterns = {std::regex(".*")};

  const auto logHandler = std::bind(&logCallback, std::placeholders::_1, std::placeholders::_2);
  auto server = foxglove::ServerFactory::createServer<foxglove::ConnHandle>(
    "client-pub-test-server", logHandler, serverOptions);

  foxglove::ServerHandlers<foxglove::ConnHandle> hdlrs;
  hdlrs.clientAdvertiseHandler = [&](const foxglove::ClientAdvertisement& adv,
                                     foxglove::ConnHandle hdl) {
    std::cout << "schema name: " << adv.schemaName << std::endl;
    std::cout << "schema def: " << adv.schema.value() << std::endl;
  };
  server->setHandlers(std::move(hdlrs));
  server->start("localhost", 8765);
  std::signal(SIGINT, signal_handler);

  while (!doAbort) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  server->stop();
  return EXIT_SUCCESS;
}
