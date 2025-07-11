#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

class UdpFeedHandler {
public:
    UdpFeedHandler(const std::string& iface_ip,
                   const std::string& multicast_ip,
                   uint16_t port,
                   bool is_multicast)
        : iface_ip_(iface_ip), multicast_ip_(multicast_ip), port_(port), is_multicast_(is_multicast) {}

    void run() {
        setupSocket();
        std::cout << "[INFO] Listening on " << (is_multicast_ ? multicast_ip_ : iface_ip_) << ":" << port_ << std::endl;

        const int epfd = epoll_create1(0);
        if (epfd == -1) throw std::runtime_error("epoll_create1 failed");

        struct epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.fd = sockfd_;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd_, &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");

        std::vector<char> buffer(4096);
        while (true) {
            struct epoll_event events[1];
            int nfds = epoll_wait(epfd, events, 1, -1);
            if (nfds == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error("epoll_wait failed");
            }

            ssize_t len = recv(sockfd_, buffer.data(), buffer.size(), 0);
            if (len > 0) {
                handlePacket(buffer.data(), static_cast<size_t>(len));
            }
        }
    }

private:
    int sockfd_ = -1;
    std::string iface_ip_;
    std::string multicast_ip_;
    uint16_t port_;
    bool is_multicast_;
    std::atomic<uint32_t> last_seq_ = 0;

    void setupSocket() {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) throw std::runtime_error("socket creation failed");

        int reuse = 1;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
            throw std::runtime_error("setsockopt SO_REUSEADDR failed");

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = is_multicast_ ? htonl(INADDR_ANY) : inet_addr(iface_ip_.c_str());
        addr.sin_port = htons(port_);

        if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("bind failed");

        if (is_multicast_) {
            ip_mreq mreq = {};
            mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip_.c_str());
            mreq.imr_interface.s_addr = inet_addr(iface_ip_.c_str());
            if (setsockopt(sockfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                throw std::runtime_error("setsockopt IP_ADD_MEMBERSHIP failed");
        }

        // Set non-blocking
        int flags = fcntl(sockfd_, F_GETFL, 0);
        fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK);
    }

    void handlePacket(const char* data, size_t len) {
        if (len < sizeof(uint32_t)) {
            std::cerr << "[WARN] Packet too short (" << len << " bytes)" << std::endl;
            return;
        }

        uint32_t seq_num;
        std::memcpy(&seq_num, data, sizeof(uint32_t));
        seq_num = ntohl(seq_num);

        uint32_t expected = last_seq_ + 1;
        if (last_seq_ != 0 && seq_num != expected) {
            std::cerr << "[LOSS] Gap detected. Expected: " << expected << ", Received: " << seq_num << std::endl;
        }

        last_seq_ = seq_num;

        // TODO: Parse application payload (data + sizeof(uint32_t), len - sizeof(uint32_t))
        std::cout << "[RECV] Seq=" << seq_num << ", Size=" << len << " bytes" << std::endl;
    }
};
