# 📚 Документация net

Оглавление. Назначение библиотеки — в [корневом README](../README.md).

| Документ | Содержание |
|----------|------------|
| [**socket-options.md**](socket-options.md) | **как настроить сокет (для начинающих)** |
| [tcp-socket.md](tcp-socket.md) | `TcpSocket` (справочник) |
| [tcp-server.md](tcp-server.md) | `TcpServer` (справочник) |
| [file-transfer.md](file-transfer.md) | передача файла и примитивы |
| [net-initializer.md](net-initializer.md) | инициализация Winsock |

```
NetInitializer  →  TcpSocket / TcpServer
TcpServer.accept  →  unique_ptr<TcpSocket>
file-transfer  →  работает поверх TcpSocket
```

Тесты: `tests/loopback.cpp` (CTest target `net-loopback`), только top-level.
