# TcpServer 🖥️

**Заголовки:** `include/net/tcp-server.h`, `include/net/server/server-options.h`  
**Реализация:** `src/net/tcp-server/tcp-server.cpp`

IPv4 listener: `listen` → `acceptConnection`.

---

## 🛠️ Настройки: `ServerOptions`

Файл: `include/net/server/server-options.h`. Одно поле `options_` на сервере.

```cpp
struct ServerOptions {
    bool reuse_address = true;   // SO_REUSEADDR на listen
    SocketOptions accepted{};    // к каждому peer после accept
};
```

`SocketOptions` / пресеты — [socket-options.md](../manuals/socket-options.md).  
Введение: [sockets-intro.md](../manuals/sockets-intro.md).

---

## ⚡ API

| Метод                           | Описание                                                 |
|---------------------------------|----------------------------------------------------------|
| `setOptions` / `options`        | вся политика сервера                                     |
| `listen(ip, port, backlog = 5)` | bind + listen                                            |
| `acceptConnection()`            | peer + `setOptions(options_.accepted)`                   |
| `close()` / `isListening()`     | закрыть listen-сокет; `true`, если сервер сейчас слушает |

---

## 💡 Пример

```cpp
#include <net/tcp-server.h>
#include <iostream>

net::ServerOptions opts;
opts.reuse_address = true;
opts.accepted      = net::make_socket_options(net::SocketPreset::Interactive);

net::TcpServer server;
server.setOptions(opts);

if (!server.listen("0.0.0.0", 50235))
    return 1;

auto client = server.acceptConnection();
if (!client)
    return 1;

std::cout << client->remoteIp() << '\n';
```

См. `examples/server.cpp`, `tests/loopback/`.
