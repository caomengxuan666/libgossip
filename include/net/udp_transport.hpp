#ifndef LIBGOSSIP_UDP_TRANSPORT_HPP
#define LIBGOSSIP_UDP_TRANSPORT_HPP

#include <cstdint>
#include <memory>
#include <string>

namespace gossip {
namespace net {

class UdpTransport {
public:
    UdpTransport(const std::string& host, uint16_t port);
    
    void start();
    void stop();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace net
} // namespace gossip

#endif // LIBGOSSIP_UDP_TRANSPORT_HPP