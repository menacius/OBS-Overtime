# Minimal bootstrap. Reads buildspec.json for project name and version.
cmake_minimum_required(VERSION 3.28...3.30)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/buildspec.json" buildspec)

string(JSON _name GET ${buildspec} name)
string(JSON _version GET ${buildspec} version)
string(JSON _website GET ${buildspec} website)
string(JSON _author GET ${buildspec} author)

set(_name ${_name} CACHE INTERNAL "Plugin internal name")
set(_version ${_version} CACHE INTERNAL "Plugin version")
set(_website ${_website} CACHE INTERNAL "Plugin website")
set(_author ${_author} CACHE INTERNAL "Plugin author")

if(POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
endif()
if(POLICY CMP0149)
  cmake_policy(SET CMP0149 NEW)
endif()
