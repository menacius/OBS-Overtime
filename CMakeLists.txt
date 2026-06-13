cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

# Avoid common Windows/OBS/Qt macro conflicts.
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
  QT_NO_KEYWORDS
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  PLUGIN_NAME="${_name}"
  PLUGIN_VERSION="${_version}"
)

# -----------------------------------------------------------------------------
# OBS discovery
# -----------------------------------------------------------------------------

find_package(libobs QUIET)
find_package(obs-frontend-api QUIET)

if(NOT TARGET OBS::libobs OR NOT TARGET OBS::obs-frontend-api)
  set(OBS_SEARCH_ROOTS)

  if(OBS_SDK_DIR)
    list(APPEND OBS_SEARCH_ROOTS "${OBS_SDK_DIR}")
  endif()

  if(DEFINED ENV{OBS_SDK_DIR})
    list(APPEND OBS_SEARCH_ROOTS "$ENV{OBS_SDK_DIR}")
  endif()

  if(DEFINED ENV{OBS_STUDIO_DIR})
    list(APPEND OBS_SEARCH_ROOTS "$ENV{OBS_STUDIO_DIR}")
  endif()

  if(WIN32)
    foreach(base_dir
        "${CMAKE_SOURCE_DIR}/.deps"
        "${CMAKE_SOURCE_DIR}/obs-build-dependencies"
        "$ENV{USERPROFILE}/Desktop/obs-build-dependencies"
        "$ENV{USERPROFILE}/Downloads/obs-build-dependencies")
      if(IS_DIRECTORY "${base_dir}")
        file(GLOB candidates
          "${base_dir}/plugin-deps-*"
          "${base_dir}/obs-studio*"
          "${base_dir}/windows-deps*"
        )
        list(APPEND OBS_SEARCH_ROOTS ${candidates})
      endif()
    endforeach()

    if(DEFINED ENV{ProgramFiles})
      list(APPEND OBS_SEARCH_ROOTS "$ENV{ProgramFiles}/obs-studio")
    endif()

    if(DEFINED ENV{ProgramW6432})
      list(APPEND OBS_SEARCH_ROOTS "$ENV{ProgramW6432}/obs-studio")
    endif()
  endif()

  list(REMOVE_DUPLICATES OBS_SEARCH_ROOTS)

  foreach(candidate IN LISTS OBS_SEARCH_ROOTS)
    if(NOT IS_DIRECTORY "${candidate}")
      continue()
    endif()

    unset(OBS_LIB CACHE)
    unset(OBS_FRONTEND_API_LIB CACHE)
    unset(OBS_INCLUDE_DIR CACHE)
    unset(OBS_DLL CACHE)
    unset(OBS_FRONTEND_API_DLL CACHE)

    find_library(OBS_LIB
      NAMES obs libobs
      HINTS "${candidate}/lib" "${candidate}/lib/obs"
      NO_DEFAULT_PATH)

    find_library(OBS_FRONTEND_API_LIB
      NAMES obs-frontend-api
      HINTS "${candidate}/lib" "${candidate}/lib/obs"
      NO_DEFAULT_PATH)

    find_path(OBS_INCLUDE_DIR
      NAMES obs.h obs-module.h
      HINTS "${candidate}/include" "${candidate}/include/obs"
      NO_DEFAULT_PATH)

    if(OBS_LIB AND OBS_FRONTEND_API_LIB AND OBS_INCLUDE_DIR)
      set(OBS_SDK_DIR "${candidate}" CACHE PATH "Path to OBS SDK/install tree" FORCE)
      message(STATUS "Found OBS SDK/install tree: ${OBS_SDK_DIR}")

      find_file(OBS_DLL
        NAMES obs.dll
        HINTS "${OBS_SDK_DIR}/bin/64bit" "${OBS_SDK_DIR}/bin"
        NO_DEFAULT_PATH)

      find_file(OBS_FRONTEND_API_DLL
        NAMES obs-frontend-api.dll
        HINTS "${OBS_SDK_DIR}/bin/64bit" "${OBS_SDK_DIR}/bin"
        NO_DEFAULT_PATH)

      if(NOT TARGET OBS::libobs)
        add_library(OBS::libobs SHARED IMPORTED GLOBAL)
        set_target_properties(OBS::libobs PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${OBS_INCLUDE_DIR}")

        if(WIN32)
          set_target_properties(OBS::libobs PROPERTIES IMPORTED_IMPLIB "${OBS_LIB}")
          if(OBS_DLL)
            set_target_properties(OBS::libobs PROPERTIES IMPORTED_LOCATION "${OBS_DLL}")
          endif()
        else()
          set_target_properties(OBS::libobs PROPERTIES IMPORTED_LOCATION "${OBS_LIB}")
        endif()
      endif()

      if(NOT TARGET OBS::obs-frontend-api)
        add_library(OBS::obs-frontend-api SHARED IMPORTED GLOBAL)
        set_target_properties(OBS::obs-frontend-api PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${OBS_INCLUDE_DIR}")

        if(WIN32)
          set_target_properties(OBS::obs-frontend-api PROPERTIES IMPORTED_IMPLIB "${OBS_FRONTEND_API_LIB}")
          if(OBS_FRONTEND_API_DLL)
            set_target_properties(OBS::obs-frontend-api PROPERTIES IMPORTED_LOCATION "${OBS_FRONTEND_API_DLL}")
          endif()
        else()
          set_target_properties(OBS::obs-frontend-api PROPERTIES IMPORTED_LOCATION "${OBS_FRONTEND_API_LIB}")
        endif()
      endif()

      break()
    endif()
  endforeach()
endif()

if(NOT TARGET OBS::libobs)
  find_package(libobs REQUIRED)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  if(NOT TARGET OBS::obs-frontend-api)
    find_package(obs-frontend-api REQUIRED)
  endif()
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core Gui REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)

  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )

  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES
      AUTOMOC ON
      AUTOUIC ON
      AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/plugin-config.cpp
    src/plugin-config.hpp
    src/time-utils.cpp
    src/time-utils.hpp
    src/media-manager.cpp
    src/media-manager.hpp
    src/projector-tracker.cpp
    src/projector-tracker.hpp
    src/overlay-window.cpp
    src/overlay-window.hpp
    src/overlay-controller.cpp
    src/overlay-controller.hpp
    src/settings-dialog.cpp
    src/settings-dialog.hpp
    src/media-dock.cpp
    src/media-dock.hpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
