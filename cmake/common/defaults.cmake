# Default project metadata definitions exposed to the plugin sources.
target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    PLUGIN_NAME=\"${_name}\"
    PLUGIN_VERSION=\"${_version}\"
)
