cmake_minimum_required(VERSION 3.20)

project(project
    VERSION 1.0.0
    LANGUAGES CXX
)

# -----------------------------
#   C++ Standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
#   Compiler warnings
# -----------------------------
if (MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# -----------------------------
#   Dependencies
# -----------------------------

# Boost (asio, beast, program_options, container)
find_package(Boost 1.82 REQUIRED COMPONENTS
    system
    thread
    coroutine
    program_options
    container
)

# OpenSSL
find_package(OpenSSL REQUIRED)

# PostgreSQL (libpqxx)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# OpenTelemetry C++ SDK
find_package(opentelemetry-cpp CONFIG REQUIRED)

# -----------------------------
#   Source files
# -----------------------------
file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
)

# -----------------------------
#   Include directories
# -----------------------------
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${PQXX_INCLUDE_DIRS}
)

# -----------------------------
#   Executable
# -----------------------------
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

# -----------------------------
#   Linking
# -----------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Boost::system
        Boost::thread
        Boost::program_options
        Boost::container
        OpenSSL::SSL
        OpenSSL::Crypto
        ${PQXX_LIBRARIES}
        opentelemetry-cpp::api
        opentelemetry-cpp::sdk
        #opentelemetry-cpp::exporter_otlp_http
)

# -----------------------------
#   Definitions
# -----------------------------
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        BOOST_ASIO_NO_DEPRECATED
	BOOST_NO_CXX17_HDR_STRING_VIEW=0
)

# -----------------------------
#   Install (optional)
# -----------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# -----------------------------
#   Output directories
# -----------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)
