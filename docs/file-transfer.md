# file-transfer 📁

**Заголовок:** `include/net/file-transfer.h`  
**Реализация:** `src/net/file-transfer.cpp`

Free functions: файл (и примитивы «ровно N байт» / `uint64`) по **уже установленному** `TcpSocket`.

Не rsync, не HTTP upload, не докачка. Протокол намеренно тупой.

---

## 📡 Протокол файла

```
[ 8 байт: file_size в network byte order (big-endian uint64) ]
[ file_size байт: содержимое файла как есть ]
```

- отправитель: open → size → тело чанками по **64 KiB**;
- получатель: size → write path → при ошибке **удаляет** частичный файл

Progress: `void(uint64_t done, uint64_t total)` — опциональный `std::function`.

Буфер 64 KiБ — **разумный default**, не max throughput (см. назначение в корневом README).

---

## 🚀 Быстрый старт

```cpp
#include <net/tcp-socket.h>
#include <net/file-transfer.h>
#include <iostream>

// sender
net::TcpSocket sock;
sock.connect("127.0.0.1", 9000);
sock.setTimeouts(60, 60); // настоятельно рекомендуется

const bool ok = net::send_file_with_progress(sock, "data.bin",
    [](std::uint64_t sent, std::uint64_t total) {
        std::cout << sent << " / " << total << '\n';
    });

// receiver (на другом процессе / сокете после accept)
net::receive_file_with_progress(*peer, "out.bin",
    [](std::uint64_t r, std::uint64_t t) {
        std::cout << r << " / " << t << '\n';
    });
```

Сигнатуры: **`TcpSocket&`** — при сбое возможен `disconnect()`, чтобы разбудить peer.

---

## 📋 API

| Функция | Назначение |
|---------|------------|
| `host_to_network64` / `network_to_host64` | endian для `uint64` (симметричны) |
| `send_uint64` / `receive_uint64` | 8 байт network order |
| `receive_exact` | читать, пока не наберётся `size` |
| `send_file_with_progress` | файл → socket |
| `receive_file_with_progress` | socket → файл |

I/O-функции — `[[nodiscard]] bool` (кроме endian helpers).

### Endian

- **C++23** (`__cpp_lib_byteswap`): `std::endian` + `std::byteswap`;
- **иначе**: ручной swap + эвристика LE;
- неизвестная платформа в fallback **считается LE**

Публичные endian-функции **не** `constexpr` (определения в `.cpp`).

---

## ⚠️ Ошибки

### Отправитель

1. Open **до** size — не анонсируем размер, который не можем прочитать.
2. После size сбой read/send → **`disconnect()`** + `false`.
3. Нет error-frame: обрыв = ошибка.

### Получатель

1. Не прочитали size → `false`.
2. Сбой mid-body → remove path, `disconnect()`, `false`.
3. Успех → flush, файл остаётся.

### Зависания

Kill -9 / unplug без `disconnect` + нет `setTimeouts` → peer может ждать **вечно**.

---

## 🚫 Ограничения

| Тема | Реальность |
|------|------------|
| Целостность | **Нет** checksum |
| Докачка | **Нет** |
| Имя / метаданные | **Не** в протоколе |
| Каталоги | только regular file |
| Max speed | userspace copy, 64 KiB, нет sendfile / TCP_NODELAY |
| Callback | `std::function`, не zero-overhead |
| Потоки | не thread-safe на одном socket |

---

## 🔧 Примитивы без файла

```cpp
std::uint8_t header[16];
if (!net::receive_exact(sock, header, 16))
    return;

std::uint64_t n = 0;
if (!net::receive_uint64(sock, n))
    return;
```

---

## 🔗 Связанные модули

- [tcp-socket.md](tcp-socket.md) — транспорт;
- [tcp-server.md](tcp-server.md) — accept
