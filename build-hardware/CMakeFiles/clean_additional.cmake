# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\appasus_control_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\appasus_control_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\hardware_rules_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\hardware_rules_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\native_monitoring_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\native_monitoring_test_autogen.dir\\ParseCache.txt"
  "appasus_control_autogen"
  "hardware_rules_test_autogen"
  "native_monitoring_test_autogen"
  )
endif()
