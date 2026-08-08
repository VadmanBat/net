# TcpServer 🖥️

**Заголовок:** `include/net/tcp-server.h`  
**Реализация:** `src/net/tcp-server/tcp-server.cpp`

IPv4 listener: `listen` → `acceptConnection`.

---

## Настройки: `ServerOptions`

Одно поле `options_` вместо россыпи accepted-*.

```cpp
struct ServerOptions {
    bool reuse_address = true;   // SO_REUSEADDR на listen
    SocketOptions accepted{};    // применяется к каждому peer после accept
};
```

`SocketOptions` — в `tcp-socket.h` (`std::optional` = «не трогать»).

**Гайд для начинающих:** [socket-options.md](socket-options.md).

---

## API

| Метод | Описание |
|-------|----------|
| `setOptions` / `options` | вся политика сервера |
| `listen(ip, port, backlog = 5)` | bind + listen |
| `acceptConnection()` | peer + `setOptions(options_.accepted)` |
| `close()` / `isListening()` | |

---

## Пример

```cpp
#include <net/tcp-server.h>
#include <iostream>

net::ServerOptions opts;
opts.reuse_address             = true;
opts.accepted.no_delay         = true;
opts.accepted.send_timeout_sec = 60;
opts.accepted.recv_timeout_sec = 60;
opts.accepted.send_buffer_size = 1 << 20;

net::TcpServer server;
server.setOptions(opts);

if (!server.listen("0.0.0.0", 50235))
    return 1;

auto client = server.acceptConnection();
if (!client)
    return 1;

std::cout << client->remoteIp() << '\n';
```

См. `examples/server.cpp`, `tests/loopback.cpp`.
