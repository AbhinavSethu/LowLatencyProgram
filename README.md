# UDP Market Data Feed Handler (C++)

A high-performance, epoll-based UDP feed handler for ingesting multicast or unicast market data in latency-sensitive trading systems.

## Features

- Supports both multicast and unicast input
- Sequence number tracking with gap detection
- Epoll-based, non-blocking I/O for high throughput
- Clean and extensible C++17 design
- Suitable for production environments

## Build

Requires:
- GCC 9+ or Clang
- Linux with multicast support

```bash
g++ -O2 -std=c++17 main.cpp -o feed_handler
./feed_handler <iface_ip> <multicast_ip> <port> <is_multicast>

