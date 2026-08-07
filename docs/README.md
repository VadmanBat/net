# 📚 Документация net

**net** — статическая C++17 библиотека блокирующего TCP и простой передачи файлов.

Документация зеркалит `include/net/`. Здесь нет маркетинга: что код делает — и чего **не** делает.

О **назначении** библиотеки (минимум vs скорость vs non-goals) — в корневом [README.md](../README.md) → раздел **«Назначение»**.

---

## 📁 Структура docs

Один модуль — один файл (папка нужна только если файлов несколько):

```
docs/
├── README.md
├── tcp-socket.md
├── tcp-server.md
├── file-transfer.md
└── net-initializer.md
```

---

## 🎯 Модули

### TcpSocket

**Файл:** [tcp-socket.md](tcp-socket.md)

TCP peer: connect, полный send, один recv, timeouts, `remoteIp()`.  
Move-only. IPv4. Блокирующий.

### TcpServer

**Файл:** [tcp-server.md](tcp-server.md)

listen + accept. `SO_REUSEADDR`. Один backlog, без пула потоков.

### file-transfer

**Файл:** [file-transfer.md](file-transfer.md)

Протокол: `uint64` size (BE) + сырые байты. Progress опционален. При сбое — `disconnect()`.

### NetInitializer

**Файл:** [net-initializer.md](net-initializer.md)

Singleton Winsock / no-op POSIX. Обычно трогать не нужно.

---

## 🏗️ Архитектура (кратко)

```
NetInitializer  ← неявно из TcpSocket / TcpServer
       ↑
  TcpSocket     ← клиент или peer после accept
       ↑
  TcpServer     ← accept → unique_ptr<TcpSocket>
       ↑
 file-transfer  ← free functions над уже открытым TcpSocket
```

I/O **синхронный**. Очереди событий внутри нет.

---

## 🧭 Как читать

| Задача | Куда |
|--------|------|
| Назначение, CMake | [../README.md](../README.md) |
| Клиент TCP | [tcp-socket.md](tcp-socket.md), `examples/client.cpp` |
| Сервер TCP | [tcp-server.md](tcp-server.md), `examples/server.cpp` |
| Файл | [file-transfer.md](file-transfer.md) |
| «Висит» recv/accept | таймауты в [tcp-socket.md](tcp-socket.md) |
| bind «address in use» | [tcp-server.md](tcp-server.md) (`SO_REUSEADDR`) |

---

## ⚠️ Общие ограничения

1. **Только IPv4.**
2. **Только TCP.**
3. **Блокирующие** сокеты — без `setTimeouts` можно ждать вечно.
4. **Нет TLS.**
5. Ошибки — `bool` / `Ssize` / `nullptr`, без текста.
6. **Не thread-safe** на одном сокете.
7. CMake ≥ 3.31, C++ 17+.
8. Throughput — **разумный default**, не max канала (см. назначение в корневом README).

Нужен async, HTTP, IPv6, TLS или max-10G — другой стек или свой слой поверх.
