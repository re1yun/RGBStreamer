#pragma once

#include <array>
#include <winsock2.h>
#include <ws2tcpip.h>

/**
 * Simple wrapper around a UDP socket for sending RGB values.
 */
class UDPSender {
public:
    /**
     * Initialize Winsock and create the UDP socket.
     * @return true on success, false otherwise.
     */
    bool open();

    /**
     * Send an RGB triple to the specified address.
     * @param addr Destination address.
     * @param rgb  Array with {R,G,B} values in range [0,255].
     * @return true if the packet was sent successfully.
     */
    bool send(const sockaddr_in& addr, const std::array<int, 3>& rgb);

    /** Close the socket and clean up Winsock. */
    void close();

private:
    SOCKET sock_ = INVALID_SOCKET; ///< UDP socket handle
    bool initialized_ = false;     ///< Whether WSAStartup succeeded
};
