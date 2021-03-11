cmake_minimum_required(VERSION 3.0)


#### Customize with your own environement
SET(VCPKG_BASE_PATH $ENV{EXTERNALS2_PATH}/windows/vs2019/VCPKG/)
####
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_BASE_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")


# Custom function to handle Windows release/debug vcpkg libs
function(AVS_FINDLIBS_VCPKG VARNAME LIBNAME)
	find_library(${VARNAME}_RELEASE NAMES ${LIBNAME} PATHS  ${VCPKG_BASE_PATH}/installed/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
	find_library(${VARNAME}_DEBUG NAMES ${LIBNAME} PATHS  ${VCPKG_BASE_PATH}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib/ NO_DEFAULT_PATH)
	set(${VARNAME} debug ${${VARNAME}_DEBUG} optimized ${${VARNAME}_RELEASE} CACHE PATH "")
endfunction()
#

#############################################
project(Live555Wrapper)

# LIVE555
set(LIVE555_INCLUDE_DIR  "${VCPKG_BASE_PATH}/installed/${VCPKG_TARGET_TRIPLET}/include" CACHE PATH "")
AVS_FINDLIBS_VCPKG(LIVE555_LIBMEDIA liveMedia)
AVS_FINDLIBS_VCPKG(LIVE555_LIBUSAGE UsageEnvironment)
AVS_FINDLIBS_VCPKG(LIVE555_LIBBASICUSAGE BasicUsageEnvironment)
AVS_FINDLIBS_VCPKG(LIVE555_LIBGROUPSOCK groupsock)
SET(LIVE555_LIBRARIES ${LIVE555_LIBMEDIA} ${LIVE555_LIBUSAGE} ${LIVE555_LIBBASICUSAGE} ${LIVE555_LIBGROUPSOCK})

# OpenSSL
FIND_PACKAGE(OpenSSL REQUIRED)

add_library(Live555Wrapper SHARED
src/MP4FileSink.h
src/MP4FileSink.cpp
src/Live555Wrapper.h
src/Live555Wrapper.cpp
)

target_include_directories(Live555Wrapper PRIVATE ${LIVE555_INCLUDE_DIR})

target_link_libraries(Live555Wrapper 
${LIVE555_LIBMEDIA} 
${LIVE555_LIBUSAGE} 
${LIVE555_LIBBASICUSAGE} 
${LIVE555_LIBGROUPSOCK}
OpenSSL::SSL
OpenSSL::Crypto
Ws2_32
)

IF(WIN32)
    target_compile_definitions(Live555Wrapper PRIVATE L555W_EXPORTS _CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)