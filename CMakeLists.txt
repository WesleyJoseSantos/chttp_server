cmake_minimum_required(VERSION 3.0.0)
project(http_server LANGUAGES C VERSION 0.1.0)

add_executable(http_server main.c)

target_link_libraries(http_server ws2_32)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
