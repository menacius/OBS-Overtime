# Helper functions used by the top-level CMakeLists.
# set_target_properties_plugin: applies plugin install layout per platform.
function(set_target_properties_plugin target)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 1 _STPP "" "" "PROPERTIES")

  if(_STPP_PROPERTIES)
    set_target_properties(${target} PROPERTIES ${_STPP_PROPERTIES})
  endif()

  if(OS_WINDOWS)
    install(TARGETS ${target} RUNTIME DESTINATION obs-plugins/64bit)
  elseif(OS_LINUX)
    install(TARGETS ${target} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/obs-plugins)
    install(
      DIRECTORY data/
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/${_name}
    )
  endif()
endfunction()

if(WIN32)
  set(OS_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
  set(OS_LINUX TRUE)
elseif(APPLE)
  set(OS_MACOS TRUE)
endif()

include(GNUInstallDirs)
