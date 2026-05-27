cmake_minimum_required(VERSION 3.20)
project(edim)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(edim src/main.c src/editor.c src/ui.c src/app.c)

target_compile_options(edim PRIVATE -Wall -Wextra -pedantic)


if(EMSCRIPTEN)
  add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

  set(SDLTTF_VENDORED ON CACHE BOOL "Use vendored SDL_ttf dependencies" FORCE)
  add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)

  set(CMAKE_EXECUTABLE_SUFFIX ".html")

  target_link_libraries(edim PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

  target_link_options(edim PRIVATE 
    "--preload-file" "${CMAKE_SOURCE_DIR}/assets@assets"
    "-sAllow_MEMORY_GROWTH=1"
  )
else()
  find_package(SDL3 REQUIRED)
  find_package(SDL3_ttf REQUIRED)

  target_link_libraries(edim PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

endif()

