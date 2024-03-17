cmake_minimum_required(VERSION 3.7)

project (Mojang2SDK)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB PROJECT_SRC CONFIGURE_DEPENDS "src/**.h" "src/**.cpp")

add_executable(Mojang2SDK ${PROJECT_SRC})
