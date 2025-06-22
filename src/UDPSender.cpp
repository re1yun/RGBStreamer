#include "UDPSender.h"

#include <cstdio>

//----------------------------------------------------------------------
// open
//----------------------------------------------------------------------
// Initialize Winsock and create a UDP socket. Any existing socket is
// closed before creating a new one.
//----------------------------------------------------------------------
bool UDPSender::open() {
    close();

    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return false;
    initialized_ = true;

    sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ == INVALID_SOCKET) {
        close();
        return false;
    }
    return true;
}

//----------------------------------------------------------------------
// send
//----------------------------------------------------------------------
// Send RGB data formatted as "R%03dG%03dB%03d\n" to the specified
// address. Retries up to 3 times on failure.
//----------------------------------------------------------------------
bool UDPSender::send(const sockaddr_in& addr,
                     const std::array<int, 3>& rgb) {
    if (sock_ == INVALID_SOCKET)
        return false;

    char buf[20];
    int len = std::snprintf(buf, sizeof(buf), "R%03dG%03dB%03d\n", rgb[0], rgb[1],
                            rgb[2]);
    if (len <= 0 || len >= static_cast<int>(sizeof(buf)))
        return false;

    int attempts = 0;
    while (attempts < 3) {
        int sent = ::sendto(sock_, buf, len, 0,
                            reinterpret_cast<const sockaddr*>(&addr),
                            sizeof(addr));
        if (sent == len)
            return true;
        ++attempts;
    }
    return false;
}

//----------------------------------------------------------------------
// close
//----------------------------------------------------------------------
// Clean up the socket and Winsock resources.
//----------------------------------------------------------------------
void UDPSender::close() {
    if (sock_ != INVALID_SOCKET) {
        ::closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
    if (initialized_) {
        WSACleanup();
        initialized_ = false;
    }
}

