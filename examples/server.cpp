#include <iostream>
#include <net/tcp-server.h>
#include <string>
#include <vector>

int main() {
    constexpr std::uint16_t port = 50235;
    const std::string listen_ip  = "0.0.0.0";

    net::TcpServer server;
    if (!server.listen(listen_ip, port)) {
        std::cerr << "Server: failed to listen on port " << port << "\n";
        return 1;
    }

    std::cout << "Server: listening on port " << port << " (waiting for client...)\n";

    const auto client_socket = server.acceptConnection();
    if (!client_socket) {
        std::cerr << "Server: accept failed\n";
        return 1;
    }

    std::cout << "Server: client connected\n";

    if (std::vector<uint8_t> data = client_socket->receiveBytes(1024); data.empty()) {
        std::cout << "Server: no data received (connection closed)\n";
    }
    else {
        const std::string message(data.begin(), data.end());
        std::cout << "Server: received message:\n" << message;
    }

    std::cout << "Server: done\n";
    std::cin.get();
    return 0;
}