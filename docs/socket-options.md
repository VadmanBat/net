# ⚙️ Как настроить сокет (для начинающих)

Этот текст — пошаговая инструкция, **без** углубления в ядро ОС.  
Настройки делаются структурой `SocketOptions` (клиент / peer) и `ServerOptions` (сервер).

---

## 1. Главная идея

Пока вы **не** вызываете `setOptions`, сокет живёт с **настройками операционной системы** — для простых программ этого часто хватает.

Настраивать имеет смысл, когда нужно:

- не ждать ответ **бесконечно** (таймауты);
- быстрее слать **маленькие** сообщения (игры, чат, команды);
- спокойнее переживать **долгие** соединения без трафика (keepalive);
- чуть лучше грузить **большие** передачи (размер буферов).

**Важно:** сначала нужно **создать соединение** (`connect` или `accept`), и только потом вызывать `setOptions`.  
На «пустом» сокете до connect настройки применить нельзя (некуда).

```
клиент:  TcpSocket → connect → setOptions → send / recv
сервер:  TcpServer.setOptions (заранее) → listen → accept
         (настройки accepted применятся к каждому новому клиенту сами)
```

---

## 2. Структура `SocketOptions`

Файл: `include/net/tcp-socket.h`.

Каждое поле — **необязательное** (`std::optional`).  
Если поле **не задали** — библиотека **не меняет** эту настройку (остаётся как у ОС).

| Поле | Тип (смысл) | Простыми словами |
|------|-------------|------------------|
| `no_delay` | да / нет | `true` — слать мелкие пакеты сразу (меньше задержка). `false` — ОС может чуть подождать и склеить данные (Nagle). |
| `keep_alive` | да / нет | `true` — ОС изредка проверяет, живо ли соединение, если долго тишина. |
| `send_buffer_size` | байты | Сколько данных ядро может **копить на отправку**. Больше — удобнее для крупных передач. |
| `recv_buffer_size` | байты | Сколько данных ядро может **копить на приём**, пока вы не прочитали. |
| `send_timeout_sec` | секунды | Сколько ждать, пока отправка «застряла». Потом операция завершится с ошибкой, а не навсегда. |
| `recv_timeout_sec` | секунды | Сколько ждать **входящих** данных. Иначе `receiveBytes` может висеть бесконечно. |

Значения буферов — **целые байты**, например:

- `64 * 1024` — 64 килобайта  
- `1 << 20` — 1 мегабайт (2²⁰)

ОС может **подправить** размер буфера (часто ставит не меньше запрошенного или удваивает) — это нормально.

---

## 3. Клиент: настроить после connect

```cpp
#include <net/tcp-socket.h>
#include <iostream>

int main() {
    net::TcpSocket sock;

    if (!sock.connect("127.0.0.1", 50235)) {
        // не вышло — можно посмотреть, почему
        std::cerr << sock.statusText();
        return 1;
    }

    // 1) создаём «бланк» настроек
    net::SocketOptions opts;

    // 2) заполняем только то, что нужно (остальное не трогаем)
    opts.no_delay         = true;   // быстрее мелкие сообщения
    opts.keep_alive       = true;   // держать долгую сессию
    opts.send_timeout_sec = 30;     // не ждать send вечно
    opts.recv_timeout_sec = 30;     // не ждать recv вечно
    opts.send_buffer_size = 256 * 1024;
    opts.recv_buffer_size = 256 * 1024;

    // 3) применяем
    if (!sock.setOptions(opts)) {
        std::cerr << "Не удалось применить часть настроек\n";
        std::cerr << sock.statusText();
        // соединение при этом обычно уже есть — можно продолжать
        // или выйти, если настройки критичны
    }

    // 4) обычная работа
    const std::uint8_t hi[] = {'h', 'i', '\n'};
    sock.sendBytes(hi, sizeof(hi));
}
```

Можно менять **одну** настройку без структуры:

```cpp
sock.setNoDelay(true);
sock.setTimeouts(30, 30);  // send_sec, recv_sec
```

`setOptions` удобнее, когда параметров несколько.

---

## 4. Сервер: настроить всех клиентов сразу

У сервера **одна** структура `ServerOptions`:

```cpp
struct ServerOptions {
    bool reuse_address = true;  // проще снова занять порт после перезапуска
    SocketOptions accepted{};   // что применить к КАЖДОМУ принятому клиенту
};
```

Задайте её **до** `listen`. При каждом `acceptConnection()` библиотека сама вызовет `setOptions` на новом сокете.

```cpp
#include <net/tcp-server.h>
#include <iostream>

int main() {
    net::ServerOptions opts;
    opts.reuse_address = true;

    // политика для peer'ов (клиентов)
    opts.accepted.no_delay         = true;
    opts.accepted.send_timeout_sec = 60;
    opts.accepted.recv_timeout_sec = 60;
    opts.accepted.send_buffer_size = 1 << 20; // 1 МиБ
    opts.accepted.recv_buffer_size = 1 << 20;

    net::TcpServer server;
    server.setOptions(opts);

    if (!server.listen("0.0.0.0", 50235)) {
        std::cerr << "listen failed\n";
        return 1;
    }

    auto client = server.acceptConnection(); // ждёт подключения
    if (!client)
        return 1;

    // у client уже стоят accepted-настройки
    std::cout << "peer: " << client->remoteIp() << '\n';

    // при желании можно донастроить конкретного клиента:
    net::SocketOptions extra;
    extra.keep_alive = true;
    client->setOptions(extra);
}
```

- `"0.0.0.0"` — слушать на всех сетевых интерфейсах ПК.  
- `"127.0.0.1"` — только с этого же компьютера (localhost).

---

## 5. Готовые «рецепты»

### А) Просто «заработало» (минимум)

Ничего не настраивать. Только `connect` / `listen` + `send` / `recv`.

Подходит для учёбы и локальных примеров. Минус: без таймаута программа может **навсегда** ждать сеть.

### Б) Обычный клиент / сервер с таймаутами (рекомендуется начать с этого)

```cpp
net::SocketOptions opts;
opts.send_timeout_sec = 30;
opts.recv_timeout_sec = 30;
sock.setOptions(opts);
```

На сервере:

```cpp
net::ServerOptions s;
s.accepted.send_timeout_sec = 30;
s.accepted.recv_timeout_sec = 30;
server.setOptions(s);
```

### В) Чат, команды, «отклик важнее»

```cpp
opts.no_delay         = true;
opts.send_timeout_sec = 10;
opts.recv_timeout_sec = 10;
```

### Г) Передача файла / крупные куски данных

```cpp
opts.no_delay         = true;          // часто удобно и здесь
opts.send_timeout_sec = 120;           // файл может идти долго
opts.recv_timeout_sec = 120;
opts.send_buffer_size = 1 << 20;       // 1 МиБ
opts.recv_buffer_size = 1 << 20;
```

Плюс функции из `file-transfer.h` (`send_file_with_progress` / `receive_file_with_progress`).

### Д) Долгое соединение «иногда тишина»

```cpp
opts.keep_alive       = true;
opts.send_timeout_sec = 60;
opts.recv_timeout_sec = 60;
```

Keep-alive **не заменяет** таймауты на `recv`: он про «мертвую» линию на уровне ОС, и срабатывает не мгновенно.

---

## 6. Частые вопросы

**Нужно ли настраивать что-то обязательно?**  
Нет. Но таймауты (`send_timeout_sec` / `recv_timeout_sec`) — самый полезный первый шаг.

**Почему `setOptions` вернул `false`?**  
Не все поля применились (нет сокета, ОС отказала). Соединение при этом может уже существовать. Смотрите:

```cpp
std::cerr << sock.statusText();
// или
std::cerr << sock.lastOsError() << '\n';
```

**Можно ли вызывать `setOptions` несколько раз?**  
Да. Каждый раз меняются только те поля, которые вы снова указали.

**Что если задать только `send_timeout_sec`, без `recv`?**  
Библиотека выставит **оба** таймаута: для отсутствующего возьмёт значение из того, что задано (чтобы пара send/recv всегда задавалась вместе в `setsockopt`).

**Буферы «не ровно 1 МБ» — это ошибка?**  
Нет. ОС сама решает итоговый размер; важно, что он «примерно такого порядка».

**Настройки сервера и клиента должны совпадать?**  
Не обязаны. Удобно, чтобы таймауты и буферы были согласованы по смыслу задачи, но это не протокол.

---

## 7. Куда смотреть дальше

| Документ | О чём |
|----------|--------|
| [tcp-socket.md](tcp-socket.md) | все методы `TcpSocket` |
| [tcp-server.md](tcp-server.md) | `TcpServer` и `ServerOptions` |
| [file-transfer.md](file-transfer.md) | передача файла |
| [../README.md](../README.md) | сборка и общая идея библиотеки |
| `examples/client.cpp`, `examples/server.cpp` | минимальный обмен |
| `tests/loopback.cpp` | рабочие примеры с `setOptions` |

---

## 8. Чеклист «я настроил сокет»

1. Подключил заголовок: `#include <net/tcp-socket.h>` (сервер — ещё `<net/tcp-server.h>`).  
2. Собрал проект с `net::net` (CMake).  
3. **Клиент:** `connect` → заполнил `SocketOptions` → `setOptions`.  
4. **Сервер:** заполнил `ServerOptions` → `setOptions` → `listen` → `accept`.  
5. Поставил хотя бы **таймауты**, если программа не должна зависать.  
6. При ошибке посмотрел `statusText()` / `lastOsError()`.
