cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

include(${CMAKE_BINARY_DIR}/build-debug/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLA`G ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/request_struct.h
	src/boost_logger.cpp
	src/boost_logger.h
	src/players.cpp
	src/players.h
  src/app.cpp
  src/app.h
)

target_link_libraries(game_server PRIVATE Threads::Threads)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost)
