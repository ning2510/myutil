cmake_minimum_required(VERSION 3.0)
project(myutil)

SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
enable_language(ASM)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++11")

set(PATH_BIN bin)
set(PATH_LIB lib)
set(PATH_EXAMPLE example)

# *.h
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/buffer)
include_directories(${PROJECT_SOURCE_DIR}/coroutineHook)
include_directories(${PROJECT_SOURCE_DIR}/coroutinePool)
include_directories(${PROJECT_SOURCE_DIR}/http)
include_directories(${PROJECT_SOURCE_DIR}/logger)
include_directories(${PROJECT_SOURCE_DIR}/mutex)
include_directories(${PROJECT_SOURCE_DIR}/net)
include_directories(${PROJECT_SOURCE_DIR}/netAddress)
include_directories(${PROJECT_SOURCE_DIR}/tcp)
include_directories(${PROJECT_SOURCE_DIR}/timer)
include_directories(${PROJECT_SOURCE_DIR}/timeWheel)

# *.cc
aux_source_directory(${PROJECT_SOURCE_DIR}/buffer BUFFER)
aux_source_directory(${PROJECT_SOURCE_DIR}/coroutineHook COR_HOOK)
aux_source_directory(${PROJECT_SOURCE_DIR}/coroutinePool COR_POOL)
aux_source_directory(${PROJECT_SOURCE_DIR}/http HTTP)
aux_source_directory(${PROJECT_SOURCE_DIR}/logger LOGGER)
aux_source_directory(${PROJECT_SOURCE_DIR}/mutex MUTEX)
aux_source_directory(${PROJECT_SOURCE_DIR}/net NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/netAddress NET_ADDR)
aux_source_directory(${PROJECT_SOURCE_DIR}/tcp TCP)
aux_source_directory(${PROJECT_SOURCE_DIR}/timer TIMER)
aux_source_directory(${PROJECT_SOURCE_DIR}/timeWheel TIME_WHEEL)

# .S
set(COXTX ${PROJECT_SOURCE_DIR}/coroutinePool/coctx_swap.S)

add_library(myutil ${BUFFER} ${COR_HOOK} ${COR_POOL} ${HTTP} ${LOGGER} ${MUTEX} ${NET} ${NET_ADDR} ${TCP} ${TIMER} ${TIME_WHEEL} ${COXTX})
install(TARGETS myutil DESTINATION ${PATH_LIB})

set(LIBS
    myutil
    pthread
    dl
)

add_subdirectory(example)