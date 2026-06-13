cmake_minimum_required(VERSION 3.20)

project(obs-overtime VERSION 0.1.0 LANGUAGES C CXX)

set(PLUGIN_NAME "obs-overtime")

# Qt 6 headers require modern C++. Force this on the actual plugin target too,
# because relying only on global CMAKE_CXX_STANDARD can be lost/overridden by
# external OBS helper files or stale build cache.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OBS_ARCH_DIR "64bit")
else()
  set(OBS_ARCH_DIR "32bit")
endif()

set(OBS_OVERTIME_PACKAGE_DIR "${CMAKE_BINARY_DIR}/${PLUGIN_NAME}" CACHE PATH "Staged OBS plugin package directory")
set(OBS_OVERTIME_PACKAGE_BIN_DIR "${OBS_OVERTIME_PACKAGE_DIR}/bin/${OBS_ARCH_DIR}")

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
        "$ENV{USERPROFILE}/Downloads/obs-build-dependencies"
        "$ENV{ProgramFiles}/obs-studio"
        "$ENV{ProgramW6432}/obs-studio")
      if(IS_DIRECTORY "${base_dir}")
        list(APPEND OBS_SEARCH_ROOTS "${base_dir}")
        file(GLOB candidates
          "${base_dir}/plugin-deps-*"
          "${base_dir}/obs-studio*"
          "${base_dir}/windows-deps*")
        list(APPEND OBS_SEARCH_ROOTS ${candidates})
      endif()
    endforeach()
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

    find_library(OBS_LIB NAMES obs libobs HINTS "${candidate}/lib" "${candidate}/lib/obs" NO_DEFAULT_PATH)
    find_library(OBS_FRONTEND_API_LIB NAMES obs-frontend-api HINTS "${candidate}/lib" "${candidate}/lib/obs" NO_DEFAULT_PATH)
    find_path(OBS_INCLUDE_DIR NAMES obs.h obs-module.h HINTS "${candidate}/include" "${candidate}/include/obs" NO_DEFAULT_PATH)

    if(OBS_LIB AND OBS_FRONTEND_API_LIB AND OBS_INCLUDE_DIR)
      set(OBS_SDK_DIR "${candidate}" CACHE PATH "Path to OBS SDK/install tree" FORCE)
      message(STATUS "Found OBS SDK/install tree: ${OBS_SDK_DIR}")

      find_file(OBS_DLL NAMES obs.dll HINTS "${OBS_SDK_DIR}/bin/${OBS_ARCH_DIR}" "${OBS_SDK_DIR}/bin" NO_DEFAULT_PATH)
      find_file(OBS_FRONTEND_API_DLL NAMES obs-frontend-api.dll HINTS "${OBS_SDK_DIR}/bin/${OBS_ARCH_DIR}" "${OBS_SDK_DIR}/bin" NO_DEFAULT_PATH)

      if(NOT TARGET OBS::libobs)
        add_library(OBS::libobs SHARED IMPORTED GLOBAL)
        set_target_properties(OBS::libobs PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OBS_INCLUDE_DIR}")
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
        set_target_properties(OBS::obs-frontend-api PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OBS_INCLUDE_DIR}")
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

if(NOT TARGET OBS::obs-frontend-api)
  find_package(obs-frontend-api REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Qt
# -----------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

set(PLUGIN_SOURCES
  src/plugin-main.cpp
  src/plugin-config.cpp
  src/time-utils.cpp
  src/media-manager.cpp
  src/projector-tracker.cpp
  src/overlay-window.cpp
  src/overlay-controller.cpp
  src/settings-dialog.cpp
  src/media-dock.cpp
)

set(PLUGIN_HEADERS
  src/plugin-config.hpp
  src/time-utils.hpp
  src/media-manager.hpp
  src/projector-tracker.hpp
  src/overlay-window.hpp
  src/overlay-controller.hpp
  src/settings-dialog.hpp
  src/media-dock.hpp
)

add_library(${PLUGIN_NAME} MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# Force C++17 on the actual target. This fixes MSVC parsing errors in Qt headers
# such as qobject.h / qtimer.h around std::is_convertible_v / QtPrivate.
target_compile_features(${PLUGIN_NAME} PRIVATE cxx_std_17)
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${OBS_OVERTIME_PACKAGE_BIN_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${OBS_OVERTIME_PACKAGE_BIN_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(MSVC)
  target_compile_options(${PLUGIN_NAME} PRIVATE /utf-8 /Zc:__cplusplus)
endif()

target_include_directories(${PLUGIN_NAME} PRIVATE src)

target_compile_definitions(${PLUGIN_NAME} PRIVATE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  PLUGIN_NAME="${PLUGIN_NAME}"
  PLUGIN_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)

foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
  string(TOUPPER "${config}" config_upper)
  set_target_properties(${PLUGIN_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${OBS_OVERTIME_PACKAGE_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${OBS_OVERTIME_PACKAGE_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/lib/${config}"
  )
endforeach()

if(EXISTS "${CMAKE_SOURCE_DIR}/data")
  add_custom_command(TARGET ${PLUGIN_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OBS_OVERTIME_PACKAGE_DIR}/data"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data" "${OBS_OVERTIME_PACKAGE_DIR}/data"
    COMMENT "Staging OBS plugin data files")
endif()

install(TARGETS ${PLUGIN_NAME}
  RUNTIME DESTINATION "${PLUGIN_NAME}/bin/${OBS_ARCH_DIR}"
  LIBRARY DESTINATION "${PLUGIN_NAME}/bin/${OBS_ARCH_DIR}"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/data")
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION "${PLUGIN_NAME}/data")
endif()
