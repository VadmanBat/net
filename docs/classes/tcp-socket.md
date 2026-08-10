# TcpSocket 🔌

**Заголовки:** `include/net/tcp-socket.h`  
(структуры: `socket-options.h`, `socket-status.h`)  
**Реализация:** `src/net/tcp-socket/`

| Файл             | Содержимое                                              |
|------------------|---------------------------------------------------------|
| `tcp-socket.cpp` | ctor/dtor/move, `connect`, `disconnect`                 |
| `setters.cpp`    | `setOptions`, `setNoDelay`, buffers, timeouts, …        |
| `getters.cpp`    | `noDelay`, `isConnected`, `remoteIp`, …                 |
| `io.cpp`         | `sendBytes`, `receiveBytes`                             |
| `status.cpp`     | `status`, `statusText`, `lastOsError`, note/clear error |

Один TCP-сокет: исходящее соединение или peer после accept.

---

## ⚡ API

| Метод                                  | Описание                                            |
|----------------------------------------|-----------------------------------------------------|
| `connect(ip, port)`                    | IPv4; `inet_pton` + `connect`; при ошибке fd закрыт |
| `disconnect()`                         | `shutdown` + close                                  |
| `setOptions(SocketOptions)`            | применить заданные `optional` поля                  |
| `setOptions(SocketPreset)`             | пресет Interactive / Bulk / LongLived               |
| `setNoDelay` / `noDelay`               | `TCP_NODELAY`                                       |
| `setKeepAlive` / `keepAlive`           | `SO_KEEPALIVE`                                      |
| `setSendBufferSize` / `sendBufferSize` | `SO_SNDBUF` (set: bytes > 0)                        |
| `setRecvBufferSize` / `recvBufferSize` | `SO_RCVBUF`                                         |
| `setTimeouts(send_sec, recv_sec)`      | `SO_SNDTIMEO` / `SO_RCVTIMEO`                       |

`SocketOptions` / `SocketPreset` — `include/net/socket/socket-options.h`.  
`SocketStatus` — `include/net/socket/socket-status.h`.

Параметры и пресеты: [socket-options.md](../manuals/socket-options.md).
Введение: [sockets-intro.md](../manuals/sockets-intro.md).

| Метод                       | Описание                                                            |
|-----------------------------|---------------------------------------------------------------------|
| `sendBytes`                 | полный send (цикл, чанки ≤ `INT_MAX`)                               |
| `receiveBytes(buf, max)`    | один `recv` → `Ssize`                                               |
| `receiveBytes(max)`         | аллокация; default max = 64 KiB                                     |
| `isConnected()`             | live-проверка (см. ниже)                                            |
| `remoteIp()`                | IPv4 peer                                                           |
| `lastOsError()`             | код ОС после последней неудачи (0 = нет); **O(1)**                  |
| `status()` / `statusText()` | полный снимок (адреса, опции, ошибка, текст); **только по запросу** |

Copy deleted, move supported. Деструктор → `disconnect()`.  
`net::Ssize` = `std::ptrdiff_t`.

Опции до `set*` не трогаются — default ОС. Getters — `getsockopt`.

**Производительность:** `send`/`recv` не форматируют сообщения. При fail — запись `int` error code. `status()` дороже (
probe + getsockopt + string) — для отладки / логов после ошибки.

---

## 🔍 `isConnected()`

1. Нет fd → false.
2. `getpeername` fail → false.
3. `select` (0 timeout): нет ready → true.
4. `recv(..., MSG_PEEK)`: `0` → peer закрыл; `>0` → true; would-block → true; иначе false.

Обновляет внутренний `connected_`. Не заменяет keepalive: «тихий» half-open без FIN может ещё считаться true, пока стек
не узнает.

---

## 💡 Пример

```cpp
#include <net/tcp-socket.h>

net::TcpSocket sock;
if (!sock.connect("127.0.0.1", 50235))
    return;

sock.setOptions(net::SocketPreset::Interactive);

const std::uint8_t payload[] = {1, 2, 3};
sock.sendBytes(payload, sizeof(payload));

if (!sock.isConnected())
    return;

std::uint8_t buf[256];
const net::Ssize n = sock.receiveBytes(buf, sizeof(buf));
```

1. `receiveBytes` — один `recv`; для ровно N байт — `net::receive_exact`.
2. Один сокет — из одного потока.
