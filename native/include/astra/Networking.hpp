#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace astra {
enum class NetworkRole { Server, Client, Host, Authority }; 
struct RpcMessage { std::uint32_t objectId=0; std::uint32_t methodId=0; std::vector<std::uint8_t> payload; };
class NetworkTransport { public: virtual ~NetworkTransport()=default; virtual bool start(std::uint16_t)=0; virtual void poll()=0; virtual void send(const std::vector<std::uint8_t>&)=0; };
class LoopbackTransport final: public NetworkTransport { std::vector<std::vector<std::uint8_t>> queue_; public: bool start(std::uint16_t) override{return true;} void poll() override{} void send(const std::vector<std::uint8_t>& p) override{queue_.push_back(p);} size_t pending()const{return queue_.size();} };
}
