cmake_minimum_required(VERSION 3.23)
set(ProjectMainName OwlAccessTerminal)
project(${ProjectMainName})

set(CMAKE_CXX_STANDARD 20)


set(BOOST_ROOT d:/CodeLib/boost_1_80_0_install_x64_v11p3/)
#set(Boost_INCLUDE_DIR d:/CodeLib/boost_1_74_0_install_x64_v11p3/include/boost-1_74)
#set(Boost_LIBRARY_DIR d:/CodeLib/boost_1_74_0_install_x64_v11p3/lib)


set(BOOST_DEBUG ON)


option(USE_DYNAMIC_BOOST "Config USE_DYNAMIC_BOOST")

add_definitions(-Wall -Wextra)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#if (NOT_USE_BOOST_THEAD)
#    set(BOOST_THEAD_MODULE "")
#    message(STATUS "NOT_USE_BOOST_THEAD !!!")
#else ()
#    set(BOOST_THEAD_MODULE "thread")
#    add_definitions(-DUSE_BOOST_THEAD)
#endif ()
set(BOOST_THEAD_MODULE "thread")
add_definitions(-DUSE_BOOST_THEAD)


if (USE_DYNAMIC_BOOST)
    message(STATUS "USE DYNAMIC BOOST !!!")
    add_definitions(-DBOOST_ALL_DYN_LINK=1)
else ()
    message(STATUS "USE STATIC BOOST !!!")
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif ()

set(Boost_USE_MULTITHREADED ON)
#find_package(Boost REQUIRED COMPONENTS  REQUIRED)
find_package(Boost 1.74.0 REQUIRED COMPONENTS system filesystem program_options json ${BOOST_THEAD_MODULE} REQUIRED)


message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")

include_directories(CommandService/CMakeLists.txt)

add_executable(${ProjectMainName}
        src/CommandService/CommandService.cpp
        src/CommandService/CommandService.h
        src/main.cpp
        )


target_link_libraries(${ProjectMainName} ${CMAKE_THREAD_LIBS_INIT})

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${ProjectMainName} ${Boost_LIBRARIES})
link_directories(${Boost_LIBRARY_DIRS})


if (WIN32)
    target_link_libraries(${ProjectMainName} wsock32 ws2_32 crypt32)
else ()
endif ()
