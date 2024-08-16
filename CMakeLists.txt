
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

project ("photo_watermark")
INCLUDE_DIRECTORIES(3rdparty/easyexif)

FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
FILE(GLOB_RECURSE 3RDPARTY_SOURCE 3rdparty/easyexif/exif.cpp
                                  3rdparty/easyexif/exif.h)

FILE(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp")

ADD_EXECUTABLE (photo_watermark ${SOURCES} ${3RDPARTY_SOURCE})

IF(CMAKE_BUILD_TYPE STREQUAL "Release" AND CMAKE_HOST_WIN32)
  set_property(TARGET photo_watermark PROPERTY WIN32_EXECUTABLE true)
endif()

TARGET_LINK_LIBRARIES(photo_watermark PRIVATE 
                      Qt6::Core
                      Qt6::Gui
                      Qt6::Widgets)
