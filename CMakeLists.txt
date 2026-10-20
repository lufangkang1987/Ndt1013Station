cmake_minimum_required(VERSION 3.21)
project(Ndt1013Station VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)
set(IRAY_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/iray")
if(NOT EXISTS "${IRAY_SDK_ROOT}/include/IRayFpdSys.h" OR
   NOT EXISTS "${IRAY_SDK_ROOT}/bin/x64/FpdSys.dll")
  message(FATAL_ERROR "Project-local iRay SDK is incomplete: third_party/iray")
endif()
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This SDK integration requires Windows x64")
endif()
qt_add_executable(Ndt1013Station src/main.cpp src/Station.cpp src/Station.h)
# Shared Qt loads QML plugins at runtime. Avoid the scanner's Windows
# non-ASCII source path issue; deployment must include the QML imports.
set_target_properties(Ndt1013Station PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN TRUE)
qt_add_resources(Ndt1013Station ui PREFIX "/" FILES qml/Main.qml)
target_include_directories(Ndt1013Station PRIVATE "${IRAY_SDK_ROOT}/include")
target_link_libraries(Ndt1013Station PRIVATE Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(Ndt1013Station PRIVATE QT_NO_CAST_FROM_ASCII)
if(MSVC)
  target_compile_options(Ndt1013Station PRIVATE /utf-8 /W4)
endif()
add_custom_command(TARGET Ndt1013Station POST_BUILD
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${IRAY_SDK_ROOT}/bin/x64" "$<TARGET_FILE_DIR:Ndt1013Station>/sdk"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${IRAY_SDK_ROOT}/work_dir/NDT1013LA" "$<TARGET_FILE_DIR:Ndt1013Station>/work_dir/NDT1013LA")
