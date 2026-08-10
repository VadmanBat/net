# NetInitializer ⚙️

**Заголовок:** `include/net/core/net-initializer.h`  
**Реализация:** `src/net/net-initializer.cpp`

Инициализация сетевого стека платформы. Обычно **не вызывается вручную** — это делают конструкторы `TcpSocket` и
`TcpServer`.

---

## 🖥️ Платформы

| Платформа | Действие                                     |
|-----------|----------------------------------------------|
| Windows   | `WSAStartup(2,2)` / `WSACleanup` (singleton) |
| POSIX     | no-op                                        |

---

## 💡 Пример

```cpp
net::NetInitializer::ensureInitialized(); // при необходимости явно
```

Один startup на процесс через static instance. Сокеты лучше создавать после старта `main`, а не как global static в
произвольном порядке TU.
