cmake_minimum_required(VERSION 3.20)
project(WebServerEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(webserver ${SRC_FILES})
target_include_directories(webserver PRIVATE "${CMAKE_SOURCE_DIR}/src")

if(WIN32)
    target_link_libraries(webserver PRIVATE ws2_32)
endif()

set(TEMPLATE_DIR "${CMAKE_SOURCE_DIR}/templates")
set(STATIC_DIR "${CMAKE_SOURCE_DIR}/public")
set(STYLES_DIR "${CMAKE_SOURCE_DIR}/styles")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
target_compile_definitions(webserver PRIVATE TEMPLATE_DIR=\"${TEMPLATE_DIR}\" STATIC_DIR=\"${STATIC_DIR}\" STYLES_DIR=\"${STYLES_DIR}\" DATA_DIR=\"${DATA_DIR}\")
