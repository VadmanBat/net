#include <iostream>
#include <net/tcp-socket.h>
#include <string>
#include <vector>

int main() {
    constexpr std::uint16_t port = 50235;
    const std::string server_ip  = "127.0.0.1";

    net::TcpSocket client;
    if (!client.connect(server_ip, port)) {
        std::cerr << "Client: failed to connect to " << server_ip << ":" << port << "\n";
        std::cerr << client.statusText();
        return 1;
    }

    net::SocketOptions opts;
    opts.no_delay         = true;
    opts.send_timeout_sec = 30;
    opts.recv_timeout_sec = 30;
    if (!client.setOptions(opts)) {
        std::cerr << "Client: setOptions failed\n";
        std::cerr << client.statusText();
        return 1;
    }

    std::cout << "Client: connected to server\n";

    const std::string message = "Hello from client!\n";
    if (const std::vector<std::uint8_t> data(message.begin(), message.end()); !client.sendBytes(data)) {
        std::cerr << "Client: failed to send data\n";
        std::cerr << client.statusText();
        return 1;
    }

    std::cout << "Client: message sent\n";
    std::cin.get();
    return 0;
}
