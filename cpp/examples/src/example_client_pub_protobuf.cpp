#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/websocket_client.hpp>
#include <foxglove/websocket/websocket_notls.hpp>

#include <google/protobuf/descriptor.pb.h>

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <regex>
#include <unordered_map>

#include "foxglove/SceneUpdate.pb.h"

// Writes the FileDescriptor of this descriptor and all transitive dependencies
// to a string, for use as a channel schema.
static std::string SerializeFdSet(const google::protobuf::Descriptor* toplevelDescriptor) {
  google::protobuf::FileDescriptorSet fdSet;
  std::queue<const google::protobuf::FileDescriptor*> toAdd;
  toAdd.push(toplevelDescriptor->file());
  std::unordered_set<std::string> seenDependencies;
  while (!toAdd.empty()) {
    const google::protobuf::FileDescriptor* next = toAdd.front();
    toAdd.pop();
    next->CopyTo(fdSet.add_file());
    for (int i = 0; i < next->dependency_count(); ++i) {
      const auto& dep = next->dependency(i);
      if (seenDependencies.find(dep->name()) == seenDependencies.end()) {
        seenDependencies.insert(dep->name());
        toAdd.push(dep);
      }
    }
  }
  return fdSet.SerializeAsString();
}

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
        .topic = "client_pub_protobuf",
        .encoding = "protobuf",
        .schemaName = foxglove::SceneUpdate::descriptor()->full_name(),
        .schemaEncoding = "protobuf",
        .schema = foxglove::base64Encode(SerializeFdSet(foxglove::SceneUpdate::descriptor())),
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