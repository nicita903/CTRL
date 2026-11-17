cmake_minimum_required(VERSION 3.16)
project(asus_control VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.10)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appasus_control
    main.cpp
    backend/SystemMonitor.h
    backend/SystemMonitor.cpp
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
    target_link_libraries(appasus_control PRIVATE wbemuuid ole32 oleaut32)
endif()
install(TARGETS appasus_control
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
