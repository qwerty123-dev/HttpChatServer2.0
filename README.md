Общее видение проекта
Проект — это высоконагруженный backend‑сервер на C++ с использованием:

Boost.Asio / Boost.Beast — асинхронный сетевой стек и HTTP/WebSocket

PostgreSQL (libpqxx) — основная реляционная база данных

OpenTelemetry — единственный источник observability (трейсы, метрики, логи)

DDD + чистая архитектура — чёткое разделение слоёв: domain, application, infrastructure, bootstrap

Собственный пул потоков + io_context_pool — управление event loop’ами

Memory pools — оптимизация аллокаций, контроль за памятью

Security middleware + rate limiting — базовая защита и контроль нагрузки

На начальном этапе разработки основная цель — заложить правильный “скелет”, чтобы:

реализация бизнес‑логики не требовала изменения архитектуры;

было понятно, куда добавлять новые подсистемы и фичи;

можно было сразу запускать сервер, логировать, смотреть метрики, делать простые запросы.


Основные цели и требования
Функциональные цели (на старте)
Поднять HTTP‑сервер с базовыми эндпоинтами:

/healthz — healthcheck;

/readyz — readiness;

минимальные зачатки API (например, user или chat).

Организовать обработку запросов через router + middleware цепочку.

Обеспечить минимальную интеграцию с PostgreSQL:

подключение к БД;

простой запрос (например, проверка подключения или простейшая таблица users).

Нефункциональные цели
Производительность:

асинхронный I/O;

минимизация аллокаций (memory pools);

собственный пул потоков поверх io_context.

Надёжность и эксплуатация:

корректный graceful shutdown;

healthchecks для оркестраторов (Kubernetes, docker-compose и т.д.);

логирование и трассировка запросов.

Расширяемость:

лёгкое добавление новых хендлеров (REST эндпоинтов);

лёгкое добавление доменных сущностей и репозиториев;

необязательность изменения инфраструктурного слоя при росте домена.

Прозрачность и наблюдаемость:

OpenTelemetry трейсы для запросов;

метрики HTTP/БД;

структурированные логи (JSON).

Архитектурные слои и их роль
1. Слой core
Задача: минимальное, но стабильное ядро HTTP‑механики и базовых абстракций, поверх которых всё остальное строится.

Основные компоненты:

http_request.h / http_response.h  
Абстракции HTTP‑запроса/ответа в терминах домена/приложения. Они не зависят от конкретного HTTP‑фреймворка (Beast), это “чистые” DTO для верхних слоёв.

http_router.h  
Отвечает за:

регистрацию маршрутов (URL + метод → handler);

роутинг входящего запроса к нужному handler’у;

вызов цепочки middleware.

middleware.h  
Определяет:

интерфейс middleware (например, handle(next, request, response) или coroutine‑похожую модель);

типы middleware: логирование, аутентификация, rate‑limit, error handling и т.д.

error.h  
Единая модель ошибок:

доменные ошибки;

инфраструктурные ошибки;

маппинг ошибок в HTTP статус/ответ.

request_context.h  
Представляет контекст запроса:

request_id;

trace_id / span (интеграция с OpenTelemetry);

информация о пользователе (если аутентифицирован);

дополнительные атрибуты (IP, user agent, scopes и т.д.).

config_loader.h  
Отвечает за загрузку конфигурации приложения из config/*.yaml:

параметры сервера (порт, количество потоков);

параметры БД;

параметры логирования;

параметры rate‑limit;

параметры безопасности.

logger.h  
Интерфейс логгера (не реализация!):

методы типа info, warn, error, debug;

принимает request_id/trace_id через request_context или контекст OTEL;

не знает о конкретной реализации (OpenTelemetry, stdout, файл и т.д.).

Идея: всё, что в core — максимально стабильное и не зависит от конкретных библиотек. Это “контракты”, а не реализации.

2. Слой bootstrap
Задача: собрать приложение, подключить все зависимости, инициализировать всё и предоставить Application с методом run().

Компоненты:

bootstrap/application.h  
Класс Application:

хранит основные подсистемы (io_context_pool, listener, pg_pool, router, logger, tracer и т.д.);

предоставляет методы:

run() — запуск main loop’ов / event loop’ов;

shutdown() — инициирование graceful shutdown (опционально).

bootstrap/bootstrap.h  
Набор функций (обычно статических), которые:

читают конфиг с помощью core::config_loader;

создают logger (реализация в infrastructure);

создают tracer/meter (otel);

создают пул потоков и io_context_pool;

создают SSL контекст (если нужен HTTPS);

создают пул PostgreSQL (pg_pool);

создают router и регистрируют базовые handlers;

объединяют всё в Application.

3. Слой infrastructure
Инфраструктура разбита на подмодульные подсистемы.

3.1. infrastructure/net
Назначение: чистый сетевой уровень.

Компоненты:

io_context_pool.h  
Управление несколькими boost::asio::io_context + пул потоков:

создаёт N io_context;

создаёт N потоков, каждый вызывает io_context.run();

предоставляет функцию получения одного io_context для задач/сессий.

listener.h  
Серверный слушатель:

принимает TCP соединения;

создаёт http_session или websocket_session;

регистрирует их в нужном io_context.

http_session.h  
Отвечает за:

приём HTTP‑запроса;

интеграцию с Boost.Beast;

преобразование в core::http_request;

передачу в router + middleware;

отправку core::http_response обратно через Beast.

websocket_session.h (если нужен WebSocket)
Управляет жизненным циклом WebSocket‑соединения.

ssl_context.h  
Обёртка над OpenSSL контекстом:

загрузка сертификатов (используя generate_certs.sh на dev);

настройка параметров TLS.

shutdown_manager.h  
Управляет:

остановкой listener’ов;

остановкой io_context_pool;

корректным завершением активных сессий;

синхронизацией с сигналами ОС (SIGINT/SIGTERM).

3.2. infrastructure/http
beast_adapter.h  
Адаптер между:

Boost.Beast сущностями (request/response)
и

core http_request / http_response.

http_utils.h  
Хелперы:

парсинг/формирование заголовков;

удобное формирование ответов (JSON, plain text и т.д.).

3.3. infrastructure/telemetry
Назначение: весь observability‑слой.

otel_tracer.h  
Инициализация и работа с OpenTelemetry tracer:

создание root span для запроса;

создание child span’ов для важных операций.

otel_meter.h  
Метрики:

HTTP latency;

количество запросов;

ошибки;

метрики БД.

otel_propagation.h  
Обеспечивает:

чтение traceparent и других заголовков из входящих запросов;

запись контекста в исходящие запросы (если будет client‑часть).

otel_structured_logger.h  
Реализация core::logger:

отправка логов в OTEL экспортёр;

fallback — логирование в stdout в JSON формате;

интеграция с request_context (request_id, trace_id).

3.4. infrastructure/util
rate_limiter.h / token_bucket.h  
Реализация алгоритмов:

Limiting по IP;

Limiting по route;

Глобальный лимит запросов.

string_utils.h  
Вспомогательные функции: тримминг, split, join, и т.д.

id_generator.h  
Генерация:

request_id;

session_id;

возможно, trace‑связанные ID.

infrastructure/util/security/
cors.h  
Логика CORS‑заголовков.

csrf.h (если потребуется, больше для форм/браузерного сценария).

content_type_validator.h  
Проверка допустимых типов контента.

ip_blocklist.h  
Примитивный firewall по IP (DoS, блокировки).

3.5. infrastructure/memory
memory_pool.h  
Базовый интерфейс/реализация пулов памяти.

arena.h  
Арена, которую можно использовать per‑request, per‑session или per‑thread.

fixed_block_pool.h  
Пул фиксированных блоков под частые аллокации (например, буферы).

Цель — снизить нагрузку на глобальный аллокатор и уменьшить фрагментацию.

4. Слой application
Назначение: сценарии использования (use cases), бизнес‑операции на уровне приложения, процессинг HTTP‑запросов в терминах домена.

4.1. application/handlers
user_handlers.h  
HTTP‑хендлеры для пользовательского API:

GET /users/{id};

POST /users;

и т.п.

auth_handlers.h  
Аутентификация/авторизация:

POST /login;

POST /logout;

refresh tokens и т.д. (по мере появления).

chat_handlers.h  
Обработка логики чата (если это целевой домен):

создание сообщений;

получение истории.

healthcheck_handler.h  
Обработчик /healthz, /readyz:

проверяет готовность БД;

проверяет состояние основных подсистем;

возвращает JSON с состоянием.

4.2. application/services
user_service.h  
Операции над пользователями:

регистрация;

поиск/обновление;

chat_service.h  
Операции чата:

создание сообщения;

получение истории;

работа с доменными событиями.

4.3. application/dto
user_dto.h, chat_dto.h  
DTO для:

сериализации запросов/ответов;

маппинга между HTTP JSON и доменными сущностями.

5. Слой domain
Назначение: чистая бизнес‑логика, не зависящая от инфраструктуры.

5.1. domain/entities
user.h  
Доменная сущность пользователя:

id (user_id VO);

username;

email;

статус;

инварианты: корректность email, уникальность username (в смысле доменной логики).

message.h  
Сообщение:

id (message_id VO);

отправитель (user_id);

текст;

timestamp;

инварианты: не пустой текст, длина, т.д.

5.2. domain/value_objects
user_id.h, message_id.h  
Value‑объекты:

инварианты на идентификатор;

невозможность “пустого” id;

удобное сравнение, хэширование.

5.3. domain/repositories
user_repository.h, message_repository.h  
Интерфейсы:

find_by_id, save, remove, т.д.

ничего не знают о БД, только о домене.

5.4. domain/events
domain_events.h  
Описание доменных событий:

UserRegistered;

MessageSent;

и т.д.

6. Слой persistence (PostgreSQL)
Назначение: реализация доменных репозиториев и управление соединениями с БД.

pg_pool.h  
Пул соединений к PostgreSQL:

получение/возврат соединения;

управление размером пула;

retry/ping логика (можно добавить позже).

pg_user_repository.h, pg_message_repository.h  
Реализация интерфейсов user_repository / message_repository:

SQL запросы;

маппинг строк БД ↔ доменные сущности.

Обвязка: tests, docker, ci, scripts
tests/
Unit‑тесты для:

domain;

application services;

отдельных утилит;

Интеграционные тесты:

API поверх HTTP (через TestClient);

взаимодействие с тестовой БД.

docker/
Dockerfile:

multi‑stage (build + runtime);

минимальный образ с бинарником;

docker-compose.yaml:

сервис приложения;

сервис PostgreSQL;

возможно, otel‑collector (для local dev).

ci/
Скрипты и конфиги для CI:

сборка;

тесты;

clang‑tidy;

cppcheck.

scripts/
run_dev.sh — удобный запуск сервера в dev‑режиме.

format.sh — автоформатирование кода.

generate_certs.sh — генерация тестовых TLS‑сертификатов.

deploy.sh — деплой (на начальном этапе может быть заготовкой).

load_test.sh — запуск нагрузочного тестирования (wrk, k6 и т.д.).
