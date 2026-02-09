# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/application_proxy_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/application_proxy_autogen.dir/ParseCache.txt"
  "application_proxy_autogen"
  )
endif()
