# NetInitializer ⚙️

**Заголовок:** `include/net/net-initializer.h`  
**Реализация:** `src/net/net-initializer.cpp`

Платформенная инициализация сети.  
**Обычно вызывать вручную не нужно** — конструкторы `TcpSocket` / `TcpServer`.

---

## ✨ Зачем

| Платформа | Что делает |
|-----------|------------|
| **Windows** | `WSAStartup(2,2)` в ctor singleton; `WSACleanup` в dtor |
| **POSIX** | no-op, `initialized_ = true` |

Winsock **обязан** быть поднят до `socket()` / `connect()`. Без этого Windows ломается загадочно; POSIX — нет.

---

## 📋 API

```cpp
namespace net {
class NetInitializer {
public:
    NetInitializer();
    ~NetInitializer();
    static void ensureInitialized(); // трогает singleton
};
}
```

- `instance_` — static data member (dynamic init);
- `ensureInitialized()` — `static_cast<void>(instance_)`

Публичный класс, но **не** everyday application API.

---

## ⚠️ Подводные камни

1. **Static init order** — глобальный `TcpSocket` в другой TU теоретически опасен; создавайте сокеты в `main`.
2. **Fail `WSAStartup`** — `initialized_` false, наружу сигнала нет.
3. **Не refcount** — один startup/cleanup; чужой `WSACleanup` может всё сломать.
4. Версия Winsock **2.2** зашита.

---

## 🛠️ Когда звать руками

Почти никогда:

```cpp
net::NetInitializer::ensureInitialized();
```

---

## 🔗 Связанные модули

- [tcp-socket.md](tcp-socket.md);
- [tcp-server.md](tcp-server.md)
