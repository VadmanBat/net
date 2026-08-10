# 📚 Документация net

Оглавление. Назначение библиотеки — в [корневом README](../README.md).

## Для понимания

| Документ                                           | Содержание                                 |
|----------------------------------------------------|--------------------------------------------|
| [**sockets-intro.md**](manuals/sockets-intro.md)   | **как работают сокет и сервер (без кода)** |
| [**socket-options.md**](manuals/socket-options.md) | **параметры настройки и пресеты**          |

## Справочник API

| Документ                                         | Содержание     |
|--------------------------------------------------|----------------|
| [tcp-socket.md](classes/tcp-socket.md)           | `TcpSocket`    |
| [tcp-server.md](classes/tcp-server.md)           | `TcpServer`    |
| [file-transfer.md](classes/file-transfer.md)     | передача файла |
| [net-initializer.md](classes/net-initializer.md) | Winsock init   |

```
NetInitializer  →  TcpSocket / TcpServer
TcpServer.accept  →  unique_ptr<TcpSocket>
file-transfer  →  поверх TcpSocket
```

Тесты: `tests/loopback/` (CTest `net-loopback`), только top-level.
