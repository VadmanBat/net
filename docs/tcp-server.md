# TcpServer 🖥️

**Заголовок:** `include/net/tcp-server.h`  
**Реализация:** `src/net/tcp-server.cpp`

`TcpServer` — минимальный IPv4 TCP listener: `listen` → (многократно) `acceptConnection`.

Нет worker-пула, нет `select` на нескольких клиентах, нет graceful drain API. Один сокет на сервер, accept отдаёт `std::unique_ptr<TcpSocket>`.

---

## ✨ Возможности

- `listen(ip, port, backlog = 5)`;
- `SO_REUSEADDR` **перед** `bind` (удобнее переживать быстрый рестарт);
- `acceptConnection()` — блокирующий `accept`;
- `close()` / деструктор;
- copy запрещён, move есть;
- `isListening()`

`NetInitializer::ensureInitialized()` вызывается в конструкторе.

---

## 🚀 Быстрый старт

```cpp
#include <net/tcp-server.h>
#include <iostream>

int main() {
    net::TcpServer server;
    // "0.0.0.0" — все интерфейсы; "127.0.0.1" — только localhost
    if (!server.listen("0.0.0.0", 50235)) {
        std::cerr << "listen failed\n";
        return 1;
    }

    // блокируется, пока не придёт клиент (или ошибка)
    auto client = server.acceptConnection();
    if (!client)
        return 1;

    std::cout << client->remoteIp() << '\n';
    auto data = client->receiveBytes(1024);
}
```

Повторный `listen` на том же объекте: если уже listening — сначала `close()`, затем новый socket.

---

## 📋 API

| Метод | Возврат | Поведение |
|-------|---------|-----------|
| `listen(ip, port, backlog)` | `bool` | socket → SO_REUSEADDR → `inet_pton` → bind → listen |
| `acceptConnection()` | `unique_ptr<TcpSocket>` | `nullptr` если не listening или accept fail |
| `close()` | void | close fd, `listening_ = false` |
| `isListening()` | `bool` | локальный флаг после успешного listen |

`inet_pton != 1` → listen fails (сокет закрывается).

---

## ♻️ SO_REUSEADDR — что даёт и чего нет

**Даёт:** чаще можно снова `bind` сразу после завершения процесса (TIME_WAIT / «address already in use»).

**Не даёт:**

- одинаковую семантику «два процесса на одном порту» на Win и Linux;
- безопасность (это не authentication)

Ошибка `setsockopt(SO_REUSEADDR)` **игнорируется** — bind всё равно пробуется.

---

## 🚫 Ограничения

1. **Один accept за раз в вашем коде** — параллель делайте сами (потоки / очередь).
2. **Блокирующий accept** — без таймаута на listen-сокете (отдельного API нет) может ждать вечно.
3. Нет `accept4` / cloexec convenience.
4. **IPv4 only.**
5. Accepted peer **не** наследует server-timeouts — `setTimeouts` на `TcpSocket` отдельно.

---

## 🩹 Типичные ошибки

| Симптом | Частая причина |
|---------|----------------|
| `listen` false | порт занят, нет прав на порт &lt; 1024, кривой IP |
| `accept` nullptr | listen не был успешен; или прерванный accept |
| «висит» | норма для blocking accept без клиента |

---

## 🔗 Связанные модули

- [tcp-socket.md](tcp-socket.md);
- examples: `examples/server.cpp`
