#include <net/tcp-socket.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
    constexpr uint16_t port     = 50235;
    const std::string server_ip = "127.0.0.1";

    net::TcpSocket client;
    if (!client.connect(server_ip, port)) {
        std::cerr << "Client: failed to connect to " << server_ip << ":" << port << "\n";
        return 1;
    }

    std::cout << "Client: connected to server\n";

    const std::string message = "Hello from client!\n";
    if (const std::vector<uint8_t> data(message.begin(), message.end()); !client.sendBytes(data)) {
        std::cerr << "Client: failed to send data\n";
        return 1;
    }

    std::cout << "Client: message sent\n";
    std::cin.get();
    return 0;
}