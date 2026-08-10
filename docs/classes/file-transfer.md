# file-transfer 📁

**Заголовок:** `include/net/file-transfer.h`  
**Реализация:** `src/net/file-transfer.cpp`

Передача файла и байтовые примитивы по уже открытому `TcpSocket`.

---

## 📋 Протокол файла

```
[8 байт: size, uint64]
[size байт: содержимое]
```

Чанки по **64 KiB**. Progress: `void(uint64_t done, uint64_t total)` (опционально).

---

## ⚡ API

| Функция                                        | Описание                         |
|------------------------------------------------|----------------------------------|
| `host_to_network64` / `network_to_host64`      | endian для `uint64`              |
| `send_uint64` / `receive_uint64`               | 8 байт в network order           |
| `receive_exact`                                | читать, пока не наберётся `size` |
| `send_file_with_progress(socket, path, cb)`    | open → size → тело               |
| `receive_file_with_progress(socket, path, cb)` | size → write path                |

`send` / `receive` file принимают `TcpSocket&`: при сбое mid-transfer вызывается `disconnect()`, peer разблокируется.

---

## 🧠 Поведение

**Отправка:** файл открывается до отправки size; при ошибке после начала протокола — `disconnect()`, `false`.

**Приём:** ошибка mid-body — удаление частичного файла, `disconnect()`, `false`; успех — flush.

Для долгих передач задайте `socket.setTimeouts(...)` или пресет `SocketPreset::Bulk`.

---

## 💡 Пример

```cpp
#include <net/tcp-socket.h>
#include <net/file-transfer.h>

net::TcpSocket sock;
sock.connect("127.0.0.1", 9000);
sock.setOptions(net::SocketPreset::Bulk);

net::send_file_with_progress(sock, "data.bin",
    [](std::uint64_t sent, std::uint64_t total) {
        // progress
    });

// на peer:
// net::receive_file_with_progress(peer, "out.bin", callback);
```

```cpp
std::uint8_t header[16];
net::receive_exact(sock, header, 16);

std::uint64_t n = 0;
net::receive_uint64(sock, n);
```
