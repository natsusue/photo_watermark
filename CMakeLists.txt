CMAKE_MINIMUM_REQUIRED (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  CMAKE_POLICY(SET CMP0141 NEW)
  SET(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC ON)

project(photo_watermark)

MESSAGE("env Qt6_DIR = $ENV{Qt6_DIR}")
MESSAGE("Qt6_DIR = ${Qt6_DIR}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
include_directories(3rdparty/easyexif ${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS})

FILE(GLOB_RECURSE 3RDPARTY_SOURCE 3rdparty/easyexif/exif.cpp
                                 3rdparty/easyexif/exif.h)
FILE(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp")
FILE(GLOB_RECURSE QRC_SOURCE_FILES "res/*.qrc")
qt_add_resources(RESOURCE_FILES ${QRC_SOURCE_FILES})

add_executable(photo_watermark ${RESOURCE_FILES} ${SOURCES} ${3RDPARTY_SOURCE})

if (CMAKE_HOST_WIN32 AND 
   (CMAKE_BUILD_TYPE MATCHES "release" OR
    CMAKE_BUILD_TYPE MATCHES "Release"))
  set_property(TARGET photo_watermark PROPERTY WIN32_EXECUTABLE true)
endif()

target_link_libraries(photo_watermark PRIVATE 
                      Qt6::Core
                      Qt6::Gui
                      Qt6::Widgets)