set(BINARY ${CMAKE_PROJECT_NAME})
set(CMAKE_BUILD_TYPE Debug)

file(GLOB_RECURSE SOURCES LIST_DIRECTORIES true *.h *.cpp)

set(SOURCES ${SOURCES})

find_library(YAML_CPP yaml-cpp REQUIRED)
find_path(LIBEVDEV libevdev-1.0 REQUIRED)
find_library(LIBEVDEV_LIB libevdev.so REQUIRED)

include_directories(${LIBEVDEV}/libevdev-1.0)

add_executable(${BINARY}_run ${SOURCES})
target_link_libraries(${BINARY}_run "${YAML_CPP}")
target_link_libraries(${BINARY}_run "${LIBEVDEV_LIB}")

add_library(${BINARY}_lib STATIC ${SOURCES})
target_link_libraries(${BINARY}_lib "${YAML_CPP}")
target_link_libraries(${BINARY}_lib "${LIBEVDEV_LIB}")


