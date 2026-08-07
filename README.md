# 🌐 net

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.31+-064F8C?style=flat&logo=cmake)](https://cmake.org/)

**net** — небольшая статическая C++ библиотека для **блокирующего TCP** (клиент / сервер) и простой передачи файлов.

---

## 🎯 Назначение

**Зачем существует:** дать в своих C++-проектах тонкий, переносимый (Windows + POSIX) слой «сделай TCP и при необходимости перешли файл», без framework'а и без зависимости от Asio / libuv / HTTP-стека.

**Какой минимум должен работать качественно:**

| Минимум | Смысл «качественно» |
|---------|---------------------|
| Клиент TCP | `connect` по IPv4, полный `send`, честный `recv`, RAII close, move-only ownership |
| Сервер TCP | `listen` + `accept` → `unique_ptr<TcpSocket>`, `SO_REUSEADDR`, cleanup на fail |
| Байты / файл | send-loop без обрезания, `receive_exact` / size+payload для файла, disconnect при сбое mid-transfer |
| Платформы | Winsock init скрыт; один API на Win и POSIX |

**Скорость:** не «выжать канал на максимум» (нет `sendfile`, `TCP_NODELAY`, тюнинга буферов, zero-copy, pipeline). Цель по throughput — **разумный default**: bulk без глупых мелких write, буфер файла 64 КиБ, минимум слоёв. Для localhost / LAN этого обычно хватает; для 10G / WAN max — отдельная работа.

**Не цель:** TLS, IPv6, async/epoll, high-load сервер, rich error codes, «магический» reconnect.

Итого одной фразой: **корректный синхронный TCP-минимум для встраивания**, а не сетевой фреймворк и не perf-чемпион.

---

## ✨ Что есть

- `TcpSocket` — connect / send / recv / timeouts / remote IP;
- `TcpServer` — listen / accept (IPv4);
- `file-transfer` — отправка/приём файла с progress callback и network-endian `uint64`;
- авто-`WSAStartup` / `WSACleanup` на Windows через `NetInitializer`

### Чего нет (и не обещается)

- TLS / шифрование;
- IPv6;
- неблокирующие сокеты, epoll/IOCP, async API;
- HTTP, WebSocket, мультиплексирование, reconnect-policy;
- unit-тесты и CI-матрица в репозитории

---

## 📦 Модули

| Модуль | Назначение | Ключевое |
|--------|------------|----------|
| **TcpSocket** | TCP-клиент / peer | connect, send loop, recv, timeouts, move-only |
| **TcpServer** | TCP-сервер | listen + SO_REUSEADDR, accept → `unique_ptr<TcpSocket>` |
| **file-transfer** | Файлы по открытому TCP | size (`uint64` BE) + payload, progress, disconnect при сбое |
| **NetInitializer** | Платформенная инициализация | singleton из конструкторов сокета/сервера |

### Платформы и стандарт

- **C++17** минимум (`cxx_std_17`); при C++23 — `std::byteswap`, иначе fallback;
- **Windows** (`ws2_32`) и **POSIX**;
- только **IPv4**

---

## 🚀 Быстрый старт

### 🛠️ Сборка (как отдельный проект)

CMake **3.31+**, C++17, внешних зависимостей нет (на Windows — `ws2_32`).

```bash
mkdir build && cd build
cmake ..
cmake --build .
# examples (только top-level):
#   server.exe / client.exe  — Windows
#   ./server   / ./client    — Unix
```

### 📦 Подключение в другой проект

```cmake
add_subdirectory(path/to/net)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE net::net)
```

Публичные заголовки: `#include <net/...>` через target `net::net`.  
При `add_subdirectory` examples **не** собираются (`PROJECT_IS_TOP_LEVEL`).

### 💡 Минимальный обмен

Сначала сервер, потом клиент (см. `examples/`).

```cpp
// server
#include <net/tcp-server.h>

int main() {
    net::TcpServer server;
    if (!server.listen("0.0.0.0", 50235))
        return 1;

    auto peer = server.acceptConnection(); // блокируется
    if (!peer)
        return 1;

    auto data = peer->receiveBytes(1024);
    // data.empty() — ошибка, таймаут, close, либо 0 байт
}
```

```cpp
// client
#include <net/tcp-socket.h>
#include <string>
#include <vector>

int main() {
    net::TcpSocket client;
    if (!client.connect("127.0.0.1", 50235))
        return 1;

    const std::string msg = "hello\n";
    const std::vector<std::uint8_t> bytes(msg.begin(), msg.end());
    return client.sendBytes(bytes) ? 0 : 1;
}
```

---

## 📁 Структура проекта

```
net/
├── include/net/
│   ├── tcp-socket.h
│   ├── tcp-server.h
│   ├── file-transfer.h
│   └── net-initializer.h
├── src/net/
│   ├── tcp-socket.cpp
│   ├── tcp-server.cpp
│   ├── file-transfer.cpp
│   └── net-initializer.cpp
├── examples/
│   ├── client.cpp
│   └── server.cpp
├── docs/
│   ├── README.md
│   ├── tcp-socket.md
│   ├── tcp-server.md
│   ├── file-transfer.md
│   └── net-initializer.md
├── CMakeLists.txt
└── README.md
```

---

## 📚 Документация

- [docs/README.md](docs/README.md) — оглавление;
- [TcpSocket](docs/tcp-socket.md);
- [TcpServer](docs/tcp-server.md);
- [file-transfer](docs/file-transfer.md);
- [NetInitializer](docs/net-initializer.md)

---

## ⚠️ Модель ошибок

Почти весь API: `bool` / `Ssize` / `nullptr` — **без** исключений и без `errno` / `WSAGetLastError` наружу.

| Результат | Смысл (примерно) |
|-----------|------------------|
| `false` / `-1` | ошибка сокета, невалидный IP, disconnect mid-transfer |
| `nullptr` | `accept` fail / сервер не listening |
| `0` от recv | peer закрыл (на части платформ таймаут может выглядеть иначе) |
| `true` / `> 0` | ок |

**Таймауты** (`setTimeouts`) — главный способ не зависнуть навсегда на blocking `accept` / `recv` / `send`.

---

## 📌 Статус

Личная / утилитарная библиотека для встраивания в свои C++ проекты.

**Фокус:** корректный синхронный TCP-минимум, Windows + POSIX, простой file transfer.  
**Не фокус:** high-load, security stack, max throughput, framework.

Баги и предложения — welcome.

---

## 📄 License

Лицензионный файл в репозитории **не зафиксирован**. Уточняйте у автора, если копируете код вовне.
