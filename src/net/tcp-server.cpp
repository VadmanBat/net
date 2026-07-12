#include "net/tcp-server.h"
#include "net/net-initializer.h"

namespace net {
TcpServer::TcpServer() {
    NetInitializer::ensureInitialized();
}

TcpServer::~TcpServer() {
    close();
}

bool TcpServer::listen(const std::string& ip, const uint16_t port, const int backlog) {
    if (listening)
        close();
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
        return false;
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);
    if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        close_socket(server_socket);
        server_socket = INVALID_SOCKET;
        return false;
    }
    if (::listen(server_socket, backlog) == SOCKET_ERROR) {
        close_socket(server_socket);
        server_socket = INVALID_SOCKET;
        return false;
    }
    listening = true;
    return true;
}

std::unique_ptr<TcpSocket> TcpServer::acceptConnection() const {
    if (!listening)
        return nullptr;
    sockaddr_in client_addr{};
#ifdef _WIN32
    int client_len = sizeof(client_addr);
#else
    socklen_t client_len = sizeof(client_addr);
#endif
    SOCKET client_socket = accept(server_socket,
                                  reinterpret_cast<sockaddr*>(&client_addr),
                                  &client_len);
    if (client_socket == INVALID_SOCKET)
        return nullptr;
    return std::make_unique<TcpSocket>(client_socket);
}

void TcpServer::close() {
    if (server_socket != INVALID_SOCKET) {
        close_socket(server_socket);
        server_socket = INVALID_SOCKET;
    }
    listening = false;
}

bool TcpServer::isListening() const {
    return listening;
}
}