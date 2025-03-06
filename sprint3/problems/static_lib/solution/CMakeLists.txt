cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(ModelLib STATIC 
	src/model.cpp src/model.h
	src/loot_generator.cpp src/loot_generator.h
	src/tagged.h
)

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
	src/loot_generator.cpp
	src/loot_generator.h
)

target_link_libraries(game_server CONAN_PKG::boost Threads::Threads ModelLib)

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 Threads::Threads ModelLib)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)
catch_discover_tests(game_server_tests)
