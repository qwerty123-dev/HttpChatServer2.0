Ниже — карта продакшен‑реализации именно как её планировал бы senior dev: по этапам, с приоритетами, зависимостями и чёткими критериями «готово». Это не просто список задач, а маршрут от пустого скелета до реально работающего сервиса в проде.


## 1. Базовая платформа: сборка, запуск, скелет
**Цель:** бинарник, который собирается, стартует и корректно завершается, без реальной бизнес‑логики.
**1.1. Инфраструктура сборки**
- **CMake:**
  - **Сделать:** убедиться, что `CMakeLists.txt` собирает проект в Debug/Release, корректно линкует Boost, OpenSSL, libpqxx, OTEL.
  - **Критерий готовности:** `cmake .. && cmake --build .` даёт рабочий бинарник без предупреждений (или минимум).
- **Структура директорий:**
  - **Сделать:** запустить твой `create_project.sh` и проверить структуру на соответствие плану.
  - **Критерий:** все папки/файлы на месте, проект собирается пустыми заглушками.
**1.2. Bootstrap + Application скелет**
- **Bootstrap:**
  - `bootstrap/bootstrap.h`, `bootstrap/bootstrap.cpp`
  - **Сделать:**
    - Заготовка `Bootstrap::create()` без реальной логики, возвращающая `std::unique_ptr<Application>`.
  - **Application:**
    - `bootstrap/application.h`, `bootstrap/application.cpp`
    - `Application::run()` пока может просто печатать “Started” и блокироваться (например, `std::this_thread::sleep_for` в заглушке).

- **main.cpp:**
  - **Сделать:** минимальный `main`:
    ```cpp
    int main() {
        auto app = project::Bootstrap::create();
        app->run();
    }
    ```
  - **Критерий:** бинарник стартует и корректно завершается по `Ctrl+C` (пока можно без сигналов, просто return).

## 2. Конфигурация и логирование
**Цель:** сервер читает конфигурацию, логирует в консоль (structured), готов к интеграции OTEL.
**2.1. Config loader**
- **Файлы:** `core/config_loader.h`, `src/core/config_loader.cpp`
- **Сделать:**
  - Определить `struct ServerConfig`, `DbConfig`, `OtelConfig`, `LoggingConfig`, `RateLimitConfig`, `SecurityConfig`.
  - Реализовать `ConfigLoader::load(path)`:
    - чтение YAML;
    - валидация обязательных полей;
    - выброс исключений при ошибках.
- **Критерий:** при неверном конфиге — чёткая ошибка при старте, при корректном — конфиг попадает в Application.
**2.2. Logger интерфейс + OTEL реализация**
- **Интерфейс:** `core/logger.h`
  - Методы: `info`, `warn`, `error`, `debug`, принимающие:
    - сообщение;
    - опциональный контекст (request_id, trace_id).
- **Реализация:** `infrastructure/telemetry/otel_structured_logger.h/.cpp`
  - На начальном этапе:
    - логирование в stdout в JSON виде: `{ "ts": ..., "level": ..., "msg": ..., "request_id": ... }`
  - Вторым шагом — интеграция с OTEL exporter.
- **Интеграция в bootstrap:**
  - Создать logger на основе `LoggingConfig`.
  - Пробросить `std::shared_ptr<Logger>` в `Application`.
- **Критерий:** любой этап запуска/инициализации пишет структурированные логи; при ошибках конфигурации — понятные сообщения.

## 3. Сетевой стек: io_context_pool, listener, HTTP session
**Цель:** настоящий HTTP‑сервер, который принимает соединения и отдаёт ответ типа “OK”.
**3.1. io_context_pool**
- **Файлы:** `infrastructure/net/io_context_pool.*`
- **Сделать:**
  - В конструкторе — создать N `boost::asio::io_context`.
  - Для каждого io_context — создать `work_guard`.
  - `start()` — создать N потоков, каждый `io_context.run()`.
  - `stop()` — `stop()` для всех io_context, join всех потоков.
  - Метод `get_executor()` или `get_io_context()` с round‑robin.
- **Критерий:** модульное тестирование: можно постить задачи в пул, они выполняются, корректно останавливаются.
**3.2. Listener**
- **Файлы:** `infrastructure/net/listener.*`
- **Сделать:**
  - Принимать в конструкторе:
    - executor/io_context;
    - endpoint;
    - factory для `http_session`.
  - `run()` — запустить async_accept loop.
  - При каждом accept:
    - создать `http_session`;
    - передать сокет;
    - вызвать `session->run()`.
- **Критерий:** при старте — сервер слушает порт, при остановке — accept loop корректно завершается.
**3.3. HTTP session (минимальная)**
- **Файлы:** `infrastructure/net/http_session.*`
- **Сделать:**
  - На старте:
    - прочитать HTTP‑запрос через Boost.Beast.
    - пока можно игнорировать реальный router и вернуть статический ответ “OK”.
- **Критерий:** `curl http://localhost:PORT` → 200 OK, тело `OK`.


## 4. Router, middleware, healthcheck
**Цель:** полноценный HTTP pipeline: router → handlers → middleware.
**4.1. Router**
- **Файлы:** `core/http_router.*`
- **Сделать:**
  - Регистрация маршрутов: `add_route(method, path, handler)`.
  - Нахождение подходящего handler по `method + path`.
  - Простой matching, без параметров на первых порах.
**4.2. Middleware**
- **Файлы:** `core/middleware.*`
- **Сделать:**
  - Интерфейс: что‑то вроде:
    ```cpp
    using Handler = std::function<void(Request&, Response&, RequestContext&)>;
    using Middleware = std::function<void(Request&, Response&, RequestContext&, const Handler& next)>;
    ```
  - Организация цепочки:
    - logger middleware;
    - error handling middleware;
    - request_id / trace middleware;
    - rate limit (позже).
**4.3. Healthcheck handler**
- **Файлы:** `application/handlers/healthcheck_handler.*`
- **Сделать:**
  - `/healthz` — просто 200 OK.
  - `/readyz` — проверка:
    - соединение с БД (простой ping).
    - состояние основных подсистем (по мере добавления).
- **Критерий:** готовность к деплою в Kubernetes/docker‑оркестратор.


## 5. OpenTelemetry: трассы, метрики, propagation
**Цель:** видеть каждый запрос и ключевые операции в трассировке/метриках.
**5.1. Tracer**
- **Файлы:** `infrastructure/telemetry/otel_tracer.*`
- **Сделать:**
  - Инициализация OTEL SDK и exporter’а (например, OTLP/HTTP).
  - Создание root span на входящий HTTP‑запрос в middleware.
  - Добавление атрибутов: метод, маршрут, статус, latency.
**5.2. Propagation**
- **Файлы:** `infrastructure/telemetry/otel_propagation.*`
- **Сделать:**
  - Извлечение контекста из заголовков (`traceparent`).
  - Связка с `request_context`.
**5.3. Метрики**
- **Файлы:** `infrastructure/telemetry/otel_meter.*`
- **Сделать:**
  - Счётчики:
    - количество запросов по маршруту/статусу;
    - ошибки.
  - Гистограммы:
    - HTTP latency.
- **Критерий:** при нагрузке через `load_test.sh` видно:
  - трассы отдельных запросов;
  - агрегированные метрики по HTTP.

## 6. PostgreSQL: пул, репозитории, simple domain
**Цель:** реальная интеграция с БД в доменном стиле.
**6.1. Pg pool**
- **Файлы:** `persistence/postgres/pg_pool.*`
- **Сделать:**
  - Инициализация пула соединений:
    - размер пула из конфига;
    - блокирующая/неблокирующая выдача соединения.
  - RAII‑обёртка для сессии (чтобы connection возвращался в пул).
**6.2. Domain repositories + PG реализации**
- **Domain:** `domain/repositories/user_repository.h`  
- **PG реализация:** `persistence/postgres/pg_user_repository.*`
  - Методы: `find_by_id`, `create`, `list`.
  - SQL минимален, но рабочий.
**6.3. Простой use case через HTTP**
- **Endpoint:** `GET /users/{id}`
  - handler → user_service → user_repository → БД.
- **Критерий:** реальный пользователь достаётся из БД, возвращается как JSON.

## 7. Security + rate limiting
**Цель:** базовая защита и управление нагрузкой.
**7.1. Security middleware**
- **Файлы:** `infrastructure/util/security/*.h`, подключаемые через middleware:
  - CORS middleware;
  - content type validator middleware;
  - basic IP blocklist.
**7.2. Rate limiter**
- **Файлы:** `infrastructure/util/rate_limiter.*`, `token_bucket.*`
- **Сделать:**
  - per‑IP limt (например, X запросов/сек).
  - глобальный лимит на `/login` и подобные чувствительные эндпоинты.
- **Интеграция:** как отдельное middleware до handler’ов.

## 8. Memory pools и производительность
**Цель:** стабилизировать аллокации и снизить нагрузку на аллокатор.
**8.1. Memory pool API**
- **Файлы:** `infrastructure/memory/*.h`
- **Сделать:**
  - per‑session arena (для временных буферов/объектов во время обработки запроса).
  - интеграция с http_session и, возможно, с router.
**8.2. Профилирование и тюнинг**
- Использовать:
  - `load_test.sh` для нагрузки;
  - perf/valgrind/heaptrack (вне кода).
- Цель:
  - убедиться в отсутствии значительных утечек;
  - понять, где выгодно применять memory pool.

## 9. Graceful shutdown и сигналы
**Цель:** корректное завершение сервера в проде.
**9.1. Shutdown manager**
- **Файлы:** `infrastructure/net/shutdown_manager.*`
- **Сделать:**
  - обработка сигналов SIGINT/SIGTERM;
  - остановка listener’а;
  - сигнализация io_context_pool’у о завершении;
  - ожидание активных сессий (опционально с таймаутом).

**9.2. Интеграция с Application**
- `Application::run()`:
  - запускает io_context_pool и listener;
  - блокируется до сигнала;
  - инициирует shutdown manager.
- **Критерий:** при SIGINT:
  - новые запросы не принимаются;
  - активные запросы дополняются;
  - процесс завершается без аварий.

## 10. Dev/CI/Docker/Prod
**Цель:** сделать проект реально разворачиваемым и поддерживаемым.
**10.1. Dev‑скрипты**
- **run_dev.sh:** сборка + запуск с dev‑конфигом.
- **format.sh:** clang‑format по всем `.h/.cpp`.
- **load_test.sh:** вызов wrk/k6 для базового сценария.
**10.2. CI**
- **ci/github-actions.yaml:**
  - шаги:
    - сборка;
    - тесты;
    - формат/clang‑tidy.
**10.3. Docker**
- Multi‑stage Dockerfile:
  - stage build: компиляция;
  - stage runtime: минимальный образ.
- docker‑compose:
  - app + postgres + otel‑collector (dev).
**10.4. Production deployment**
- **deploy.sh:**  
  - сборка в Release;
  - упаковка;
  - деплой (scp/rsync/Docker push);
  - перезапуск сервиса (systemd или контейнер).
## Как этим пользоваться
Если упростить:

1. **Этап 1–2:** скелет, конфиг, логирование → бинарник стартует, пишет логи.  
2. **Этап 3–4:** сетевой стек + router + healthcheck → реальный HTTP‑сервер.  
3. **Этап 5:** OpenTelemetry → наблюдаемость.  
4. **Этап 6:** PostgreSQL + минимальный домен → реальные данные.  
5. **Этап 7–8:** security, rate limit, memory pools → production‑grade поведение.  
6. **Этап 9–10:** graceful shutdown, CI, Docker, deploy → готовность к продакшену.

Если хочешь, могу взять любой этап (например, **Этап 3: io_context_pool + listener + http_session**) и расписать уже конкретные классы, их интерфейсы и примерный код.
