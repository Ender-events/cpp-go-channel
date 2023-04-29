cmake_minimum_required(VERSION 3.14)

project(cpp-go-channel
  VERSION 1.0
  DESCRIPTION "Go style channel in cpp"
  LANGUAGES CXX
)
option(SANITIZE_ADDRESS "Enable address sanitizer" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(simple
    simple.cpp
)
target_compile_features(simple PUBLIC cxx_std_20)

if(SANITIZE_ADDRESS)
    target_compile_options(simple PUBLIC "-fsanitize=address")
    target_link_options(simple PUBLIC "-fsanitize=address")
endif()
