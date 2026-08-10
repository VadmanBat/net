# 🌐 net

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C%2B%2B-17+-00599C?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.21+-064F8C?style=flat&logo=cmake)](https://cmake.org/)

**net** — статическая мини-библиотека: тонкая оболочка над блокирующим TCP (Windows / POSIX) и простой передачей
файлов.

---

## 🎯 Назначение

Предоставить **простейшие функции** для сетевого взаимодействия: установить TCP-соединение, слушать порт, принять
клиента, отправить и принять байты, при необходимости передать файл.

Библиотека — **базовый слой**, а не сетевой фреймворк. Поверх неё можно строить следующее:

- протоколы сообщений;
- TLS, HTTP / WebSocket;
- неблокирующий I/O, пулы соединений;
- тюнинг TCP для максимальной производительности;
- серверы с множеством клиентов и своей моделью потоков.

**Качество минимума:** полный `send`, cleanup fd, `inet_pton`, RAII, move-only, Winsock init, опции (`NODELAY`, buffers,
timeouts), live `isConnected`, `receive_exact` и file size+payload.

**Скорость:** разумный default (send-loop, буфер файла 64 KiБ).

---

## ✨ Модули

| Документ                                           | Роль                        |
|----------------------------------------------------|-----------------------------|
| [🌐 Введение](docs/manuals/sockets-intro.md)       | как устроены сокет и сервер |
| [⚙️ Параметры](docs/manuals/socket-options.md)     | поля, смысл, пресеты        |
| [🔌 Сокет](docs/classes/tcp-socket.md)             | класс `TcpSocket`           |
| [🖥️ Сервер](docs/classes/tcp-server.md)           | класс `TcpServer`           |
| [📁 Передача файла](docs/classes/file-transfer.md) | свободные функции           |

- C++17;
- IPv4, TCP, блокирующий I/O;
- Windows (`ws2_32`) и POSIX

Подробнее: [docs/README.md](docs/README.md).

---

## 🚀 Быстрый старт

### 🛠️ Сборка

CMake **3.21+**, C++17.

```bash
mkdir build && cd build
cmake ..
cmake --build .
# top-level: server / client (examples)
```

### ✈️ В другой проект

```cmake
add_subdirectory(path/to/net)
target_link_libraries(my_app PRIVATE net::net)
```

Подключение: `#include <net/...>`.

### 💡 Примеры

```cpp
// server
#include <net/tcp-server.h>

net::TcpServer server;
server.listen("0.0.0.0", 50235);
auto peer = server.acceptConnection();
auto data = peer->receiveBytes(1024);
```

```cpp
// client
#include <net/tcp-socket.h>
#include <vector>

net::TcpSocket client;
client.connect("127.0.0.1", 50235);
const std::vector<std::uint8_t> msg{'h','i','\n'};
client.sendBytes(msg);
```

См. `examples/client.cpp`, `examples/server.cpp`.

---

## 📁 Структура

```
net/
├── include/net/          # tcp-socket.h, tcp-server.h, …
├── src/net/
│   ├── tcp-socket/       # tcp-socket, setters, getters, io
│   ├── tcp-server/       # tcp-server.cpp
│   ├── file-transfer.cpp
│   └── net-initializer.cpp
├── examples/
├── docs/
├── cmake/
│   ├── sources.cmake      # список .cpp библиотеки
│   └── top-level.cmake    # examples + tests (только top-level)
├── CMakeLists.txt
└── README.md
```

---

## 🧪 Тесты

Loopback на `127.0.0.1` (один ПК, server + client в потоках).

```bash
cmake --build build
ctest --test-dir build --output-on-failure
# или: ./net-tests   /  net-tests.exe
```

Сценарии: connect fail, echo, `receive_exact` / `uint64`, опции сокета, `isConnected` после close peer, file-transfer.

---

## ⚠️ Ошибки и блокировки

Горячий путь (`send` / `recv`) **НЕ** строит строки и **НЕ** логирует. При сбое syscall сохраняется только код ОС (
`lastOsError()` — дешёво).

Подробный отчёт — **по запросу**:

```cpp
if (!sock.connect(ip, port))
    std::cerr << sock.statusText(); // или sock.status()
```

| Результат      | Типичный смысл                            |
|----------------|-------------------------------------------|
| `false` / `-1` | сбой; смотри `lastOsError()` / `status()` |
| `nullptr`      | accept не удался                          |
| `0` (recv)     | peer закрыл соединение                    |
| `true` / `> 0` | успех                                     |

Вызовы блокирующие: `TcpSocket::setTimeouts`.

---

## 📌 Статус

Утилитарная библиотека для встраивания в C++-проекты.

## 📄 License

[MIT](LICENSE) — Copyright (c) 2025 Vadim.
