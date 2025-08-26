#include "net/udp_transport.hpp"
#include <asio.hpp>

namespace gossip {
namespace net {

class UdpTransport::Impl {
public:
    Impl(const std::string& host, uint16_t port)
        : io_context_(),
          socket_(io_context_),
          endpoint_(asio::ip::make_address(host), port) {
    }
    
    void start() {
        socket_.open(endpoint_.protocol());
        socket_.bind(endpoint_);
    }
    
    void stop() {
        if (socket_.is_open()) {
            socket_.close();
        }
    }

private:
    asio::io_context io_context_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint endpoint_;
    
    friend class UdpTransport;
};

UdpTransport::UdpTransport(const std::string& host, uint16_t port)
    : pimpl_(std::make_unique<Impl>(host, port)) {
}

void UdpTransport::start() {
    pimpl_->start();
}

void UdpTransport::stop() {
    pimpl_->stop();
}

} // namespace net
} // namespace gossip