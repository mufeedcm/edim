cmake_minimum_required(VERSION 3.20)
project(edim VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(SRC_FILES 
  src/main.c
  src/editor.c
  src/ui.c
  src/app.c
)

add_executable(edim ${SRC_FILES} )
target_compile_options(edim PRIVATE -Wall -Wextra -pedantic)

if(EMSCRIPTEN)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL) 
  set(SDLTTF_VENDORED ON CACHE BOOL "Use vendored SDL_ttf dependencies" FORCE)
  add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  target_link_libraries(edim PRIVATE SDL3::SDL3-static SDL3_ttf::SDL3_ttf-static)

  target_link_options(edim PRIVATE 
    "--preload-file" "${CMAKE_SOURCE_DIR}/assets@assets"
    "-sAllow_MEMORY_GROWTH=1"
  )
else()
  option(USE_VENDORED "Build dependencies from source" OFF)
  add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:edim>/assets
  )

  if(USE_VENDORED)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    set(SDL_GPU OFF CACHE BOOL "" FORCE)
    set(SDL_RENDER_GPU OFF CACHE BOOL "" FORCE)

    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    set(SDLTTF_VENDORED ON CACHE BOOL "Use vendored SDL_ttf dependencies" FORCE)
    add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)
    target_link_libraries(edim PRIVATE SDL3::SDL3-static SDL3_ttf::SDL3_ttf-static)
  else()
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)
    target_link_libraries(edim PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
  endif()

  if(APPLE)
    set_target_properties(edim PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME "EDIM"
      MACOSX_BUNDLE_GUI_IDENTIFIER "com.mufeedcm.edim"
      MACOSX_BUNDLE_ICON_FILE "logo.icns"
    )
    set(APP_ICON assets/logo.icns)
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(edim PRIVATE ${APP_ICON})

    install(TARGETS edim BUNDLE DESTINATION .)
    install(DIRECTORY assets DESTINATION EDIM.app/Contents/Resources)

  else()
    install(TARGETS edim DESTINATION .)
    install(DIRECTORY assets DESTINATION .)
  endif()

  if(WIN32)
    target_sources(edim PRIVATE assets/resources.rc)
  endif()

endif()


set(CPACK_PACKAGE_NAME "edim")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A minimal editor written in C")
set(CPACK_PACKAGE_VENDOR "mufeedcm")

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
  set(CPACK_GENERATOR "NSIS")
else()
  set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)





