cmake_minimum_required(VERSION 3.20)
project(Adrenoline VERSION 0.1.0 LANGUAGES C)

option(AL_BUILD_TESTS "Build host tests" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(adrenoline STATIC
  src/core/status.c
  src/core/backend_registry.c
  src/home/home.c
  backends/stub/stub_backend.c
)

target_include_directories(adrenoline PUBLIC include)
target_compile_features(adrenoline PUBLIC c_std_11)
target_compile_options(adrenoline PRIVATE -Wall -Wextra -Werror)

# Backends are added here as they become real. They are not built by default
# because they require a Mesa checkout and a target kernel.
# add_subdirectory(backends/gallium)
# add_subdirectory(backends/pm4_a5xx)

if(AL_BUILD_TESTS)
  enable_testing()
  add_executable(al_smoke tests/smoke.c)
  target_link_libraries(al_smoke PRIVATE adrenoline)
  target_compile_options(al_smoke PRIVATE -Wall -Wextra -Werror)
  add_test(NAME smoke COMMAND al_smoke)

  add_executable(al_home_test tests/test_home.c)
  target_link_libraries(al_home_test PRIVATE adrenoline)
  target_compile_options(al_home_test PRIVATE -Wall -Wextra -Werror)
  add_test(NAME home COMMAND al_home_test)

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME img4tool
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_img4tool.py)
  endif()
endif()
