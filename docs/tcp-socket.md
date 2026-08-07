# TcpSocket 🔌

**Заголовок:** `include/net/tcp-socket.h`  
**Реализация:** `src/net/tcp-socket.cpp`

`TcpSocket` — обёртка над одним TCP-сокетом: клиент (`connect`) или peer после `TcpServer::acceptConnection`.

Это **не** connection pool, **не** async socket и **не** stream с буферизацией приложения.

---

## ✨ Возможности

- `connect(ip, port)` / `disconnect()`;
- полный `sendBytes` (цикл, пока не уйдёт всё или ошибка);
- `receiveBytes` — **один** системный `recv` (не «дочитать до N»);
- `setTimeouts(send_sec, recv_sec)` — `SO_SNDTIMEO` / `SO_RCVTIMEO`;
- `remoteIp()` — IPv4 peer через `getpeername` + `inet_ntop`;
- copy **запрещён**, move **есть**;
- деструктор вызывает `disconnect()` (shutdown + close)

Тип `net::Ssize` (`std::ptrdiff_t`) — возврат recv: число байт или `-1`.

---

## 🚀 Быстрый старт

```cpp
#include <net/tcp-socket.h>
#include <vector>
#include <cstdint>

net::TcpSocket sock;
if (!sock.connect("127.0.0.1", 50235))
    return; // невалидный IP, отказ connect, нет маршрута, ...

sock.setTimeouts(/*send*/ 30, /*recv*/ 30);

const std::uint8_t payload[] = {1, 2, 3};
if (!sock.sendBytes(payload, sizeof(payload)))
    return;

std::uint8_t buf[256];
const net::Ssize n = sock.receiveBytes(buf, sizeof(buf));
if (n < 0) {
    // ошибка / (часто) таймаут
} else if (n == 0) {
    // peer закрыл (на части платформ таймаут тоже может выглядеть иначе — проверяйте)
} else {
    // n байт в buf
}
```

Векторный recv аллоцирует `max_size`, затем `resize` до фактически принятого:

```cpp
auto chunk = sock.receiveBytes(4096); // default max = 64 KiB
```

Default **64 KiB**, не мегабайт — чтобы случайный вызов не жрал кучу впустую.

---

## 📋 API (сжато)

| Метод | Возврат | Поведение |
|-------|---------|-----------|
| `connect(ip, port)` | `bool` | IPv4 only; `inet_pton` обязан вернуть 1; при fail сокет закрыт |
| `disconnect()` | `bool` (всегда true) | `shutdown(RDWR)` + close; идемпотентно |
| `setTimeouts(send, recv)` | `bool` | секунды → Win: мс `DWORD`; POSIX: `timeval` |
| `sendBytes(ptr, size)` | `bool` | цикл; чанки ≤ `INT_MAX`; size==0 → true при connected |
| `sendBytes(vector)` | `bool` | делегирует в ptr/size |
| `receiveBytes(buf, max)` | `Ssize` | **один** `recv`; max чанкуется до `INT_MAX` |
| `receiveBytes(max)` | `vector` | аллокация max, resize по факту; пустой = ошибка/0 байт |
| `isConnected()` | `bool` | флаг библиотеки, **не** live-probe TCP |
| `remoteIp()` | `string` | `""` если не connected / getpeername fail |

Конструктор `TcpSocket(SOCKET)` — для accept: помечает `connected_ = true` **без** проверки, что fd жив.

---

## ⚠️ Важные нюансы

### `receiveBytes` ≠ `receive_exact`

Один вызов recv может вернуть меньше, чем вы ждали. Для «ровно N байт» — `net::receive_exact` в [file-transfer.md](file-transfer.md).

### `isConnected()` врёт после half-close peer'а

Флаг ставится при `connect`/`accept` и сбрасывается при `disconnect`.  
Обрыв сети или `close` с той стороны **сами** флаг не сбрасывают — узнаете на следующем send/recv.

### `sendBytes` и нулевой указатель

При `size > 0` и `data == nullptr` → `false`. При `size == 0` отправка не делается, `true`, если `connected_`.

### Таймауты

Единственный встроенный анти-hang. «Таймаут vs close» на Win/POSIX слегка различается; ошибка recv → `-1`, close peer часто → `0`.  
**Не полагайтесь** на тонкое различие без проверки на целевой ОС.

### Потоки

Один `TcpSocket` из нескольких потоков без внешней блокировки — data race / UB. Не делайте так.

---

## 🚫 Ограничения

| Есть | Нет |
|------|-----|
| IPv4 | IPv6, dual-stack |
| блокирующий TCP | non-blocking, poll/select API |
| move ownership | copy, shared ownership внутри класса |
| raw bytes | message framing (кроме своего) |
| `bool` / `Ssize` | исключения, `std::error_code`, текст ошибки |

---

## 🔗 Связанные модули

- [tcp-server.md](tcp-server.md) — accept;
- [file-transfer.md](file-transfer.md) — `receive_exact`, файлы;
- [net-initializer.md](net-initializer.md) — Winsock
