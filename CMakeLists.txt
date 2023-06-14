cmake_minimum_required(VERSION 3.0)



project(Live555Wrapper)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "My multi config types" FORCE)

IF(MSVC)
	set(CMAKE_DEBUG_POSTFIX D)
	ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ELSE(MSVC)
	SET(CMAKE_CXX_FLAGS "-O3 -Wno-deprecated -Wno-invalid-offsetof -Wno-unused-result -fext-numeric-literals -Wno-psabi -fpermissive")
	SET(CMAKE_C_FLAGS "-O3 -Wno-deprecated -Wno-unused-result -Wno-psabi")
ENDIF(MSVC)

# LIVE555
find_package(unofficial-live555 CONFIG REQUIRED)
SET(LIVE555_LIBRARIES ${LIVE555_LIBMEDIA} ${LIVE555_LIBUSAGE} ${LIVE555_LIBBASICUSAGE} ${LIVE555_LIBGROUPSOCK})

# OpenSSL
FIND_PACKAGE(OpenSSL REQUIRED)

add_library(Live555Wrapper SHARED
src/MP4FileSink.h
src/MP4FileSink.cpp
src/Live555Wrapper.h
src/Live555Wrapper.cpp
src/StdoutUsageEnv.h
src/StdoutUsageEnv.cpp
)

target_include_directories(Live555Wrapper PRIVATE ${LIVE555_INCLUDE_DIR})

target_link_libraries(Live555Wrapper
unofficial::live555::liveMedia
unofficial::live555::UsageEnvironment
unofficial::live555::BasicUsageEnvironment
unofficial::live555::groupsock   
OpenSSL::SSL
OpenSSL::Crypto
Ws2_32
)

IF(WIN32)
    target_compile_definitions(Live555Wrapper PRIVATE L555W_EXPORTS _CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

add_subdirectory(test)

#### Install part
install(TARGETS Live555Wrapper
    EXPORT live555wrapper-export
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include/
)

install(FILES "src/Live555Wrapper.h" DESTINATION "include" CONFIGURATIONS Release)

install(FILES cmake/unofficial-live555wrapper-config.cmake DESTINATION "share/unofficial-live555wrapper")

install(EXPORT live555wrapper-export
  FILE
  	Live555WrapperTargets.cmake
  NAMESPACE
  	unofficial::live555wrapper::
  DESTINATION
    share/unofficial-live555wrapper
)