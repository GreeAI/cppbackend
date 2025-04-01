cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_model STATIC
	src/model.cpp src/model.h
	src/loot_generator.cpp src/loot_generator.h
	src/model_serialization.h
	src/tagged.h
	src/geom.h
)
target_link_libraries(game_model PUBLIC CONAN_PKG::boost Threads::Threads)

add_library(collision_detection_lib STATIC
	src/collision_detector.h
	src/collision_detector.cpp
	src/geom.h
)
target_link_libraries(collision_detection_lib PUBLIC CONAN_PKG::boost Threads::Threads)


add_executable(game_server 
	src/main.cpp
	src/request_struct.h
	src/http_server.cpp src/http_server.h
	src/sdk.h 
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h src/json_loader.cpp
	src/request_handler.cpp src/request_handler.h
	src/players.cpp src/players.h
	src/connection_pool.cpp src/connection_pool.h
	src/app.cpp src/app.h
	src/boost_logger.cpp src/boost_logger.h
)
target_link_libraries(game_server game_model collision_detection_lib CONAN_PKG::libpqxx)