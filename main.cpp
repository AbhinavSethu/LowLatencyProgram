#include <iostream>
#include "UdpFeedHandler.cpp"

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <iface_ip> <multicast_ip> <port> <multicast: 1|0>\n";
        return 1;
    }

    std::string iface_ip = argv[1];
    std::string multicast_ip = argv[2];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));
    bool is_multicast = std::stoi(argv[4]) != 0;

    try {
        UdpFeedHandler handler(iface_ip, multicast_ip, port, is_multicast);
        handler.run();
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
