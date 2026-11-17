cmake_minimum_required(VERSION 3.16)
project(asus_control VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.12 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.12)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appasus_control
    main.cpp
    backend/SystemMonitor.h
    backend/SystemMonitor.cpp
    backend/BatteryMonitor.h
    backend/BatteryMonitor.cpp
    backend/BatteryReadings.h
    hardware/AsusSensors.h
    hardware/AsusSensors.cpp
    hardware/HardwareCapabilities.h
    hardware/AsusProtocol.h
    hardware/AsusWmiController.h
    hardware/AsusWmiController.cpp
    hardware/FanController.h
    hardware/FanController.cpp
    hardware/CpuController.h
    hardware/CpuController.cpp
    hardware/NvidiaController.h
    hardware/NvidiaController.cpp
    hardware/HardwareManager.h
    hardware/HardwareManager.cpp
)
qt_add_qml_module(appasus_control
    URI asus_control
    QML_FILES Main.qml
)
set_target_properties(appasus_control PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
target_link_libraries(appasus_control PRIVATE Qt6::Quick)
if (WIN32)
    target_link_libraries(appasus_control PRIVATE setupapi powrprof wbemuuid ole32 oleaut32)
    target_compile_definitions(appasus_control PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

include(CTest)
if (BUILD_TESTING AND WIN32)
    qt_add_executable(native_monitoring_test tests/NativeMonitoringTest.cpp)
    target_include_directories(native_monitoring_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(native_monitoring_test PRIVATE Qt6::Core)
    target_compile_definitions(native_monitoring_test PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    add_test(NAME native_monitoring_readings COMMAND native_monitoring_test)
    qt_add_executable(hardware_rules_test tests/HardwareRulesTest.cpp hardware/FanController.cpp hardware/AsusWmiController.cpp)
    target_include_directories(hardware_rules_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(hardware_rules_test PRIVATE Qt6::Core wbemuuid ole32 oleaut32)
    target_compile_definitions(hardware_rules_test PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    add_test(NAME hardware_safety_rules COMMAND hardware_rules_test)
endif()
install(TARGETS appasus_control
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
