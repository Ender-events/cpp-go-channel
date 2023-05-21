cmake_minimum_required(VERSION 3.14)

project(cpp-go-channel
  VERSION 1.0
  DESCRIPTION "Go style channel in cpp"
  LANGUAGES CXX
)
option(SANITIZE_ADDRESS "Enable address sanitizer" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(simple
    simple.cpp
)
target_compile_features(simple PUBLIC cxx_std_20)
target_compile_options(simple PUBLIC -Wall -Wextra -Wpedantic -Werror)

if(SANITIZE_ADDRESS)
    target_compile_options(simple PUBLIC "-fsanitize=address")
    target_link_options(simple PUBLIC "-fsanitize=address")
endif()

add_executable(simple_symmetric_transfer
    simple_symmetric_transfer.cpp
)
target_compile_features(simple_symmetric_transfer PUBLIC cxx_std_20)
target_compile_options(simple_symmetric_transfer PUBLIC -Wall -Wextra -Wpedantic -Werror)

if(SANITIZE_ADDRESS)
    target_compile_options(simple_symmetric_transfer PUBLIC "-fsanitize=address")
    target_link_options(simple_symmetric_transfer PUBLIC "-fsanitize=address")
endif()
