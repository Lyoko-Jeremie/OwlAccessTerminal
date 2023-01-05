cmake_minimum_required(VERSION 3.22)
set(ProjectMainName OwlAccessTerminal)
project(${ProjectMainName})

set(CMAKE_CXX_STANDARD 20)


if (DEFAULT_CONFIG)
    # ===================== vvvvvv config segment vvvvvv =================================

    #set(OpenCV_DIR d:/CodeLib/opencv-4.5.5-build/)
    set(OpenCV_DIR d:/CodeLib/opencv-4.5.5-build-install/)
    set(OpenCV_STATIC ON)
    set(OpenCV_WORLD ON)

    set(BOOST_ROOT d:/CodeLib/boost_1_81_0_install_x64_v11p3/)
    #set(BOOST_ROOT d:/CodeLib/boost_1_80_0_install_x64_v11p3/)
    #set(Boost_INCLUDE_DIR d:/CodeLib/boost_1_74_0_install_x64_v11p3/include/boost-1_74)
    #set(Boost_LIBRARY_DIR d:/CodeLib/boost_1_74_0_install_x64_v11p3/lib)
    #set(USE_DYNAMIC_BOOST true)
    #set(BOOST_DEBUG ON)
    set(PROTOBUFF_INCLUDE_DIRS d:/CodeLib/protobuf-cpp-3.21.11-build-install/include/)
    set(PROTOBUFF_LIBS d:/CodeLib/protobuf-cpp-3.21.11-build-install/lib/libprotobuf.a)

    #add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

    # ===================== ^^^^^^ config segment ^^^^^^ =================================
endif ()


option(BOOST_ROOT "")

option(OpenCV_DIR "")

option(PROTOBUFF_INCLUDE_DIRS "")
option(PROTOBUFF_LIBS "")

message(STATUS "PROTOBUFF_INCLUDE_DIRS: ${PROTOBUFF_INCLUDE_DIRS}")
message(STATUS "PROTOBUFF_LIBS: ${PROTOBUFF_LIBS}")

option(USE_DYNAMIC_BOOST "Config USE_DYNAMIC_BOOST")

# -O1 to resolve "string table overflow at offset" error on mingw
add_definitions(-Wall -Wextra -O1 -g)

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
find_package(Boost 1.81.0 REQUIRED COMPONENTS system filesystem program_options json log ${BOOST_THEAD_MODULE} REQUIRED)

if (OpenCV_WORLD)
    find_package(OpenCV REQUIRED world)
elseif ()
    find_package(OpenCV REQUIRED)
endif ()

message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBRARY_DIRS: ${OpenCV_LIBRARY_DIRS}")
message(STATUS "OpenCV_LIBRARIES: ${OpenCV_LIBRARIES}")
message(STATUS "OpenCV_VERSION: ${OpenCV_VERSION}")
message(STATUS "OpenCV ARCH: ${OpenCV_ARCH}")
message(STATUS "OpenCV RUNTIME: ${OpenCV_RUNTIME}")
message(STATUS "OpenCV STATIC: ${OpenCV_STATIC}")

message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
message(STATUS "Boost_VERSION: ${Boost_VERSION}")

include_directories(CommandService/CMakeLists.txt)

add_executable(${ProjectMainName}
        src/main.cpp
        src/CommandService/CommandService.cpp
        src/CommandService/CommandService.h
        src/WebControlService/EmbedWebServer/EmbedWebServer.h
        src/WebControlService/EmbedWebServer/EmbedWebServer.cpp
        src/WebControlService/EmbedWebServer/EmbedWebServerSession.h
        src/WebControlService/EmbedWebServer/EmbedWebServerSession.cpp
        src/WebControlService/EmbedWebServer/EmbedWebServerTools.h
        src/WebControlService/EmbedWebServer/EmbedWebServerTools.cpp
        src/ImageService/ImageProtobufDefine/ImageProtocol/ImageProtocol.pb.h
        src/ImageService/ImageProtobufDefine/ImageProtocol/ImageProtocol.pb.cc
        src/ImageService/protobuf_test.h
        src/ImageService/protobuf_test.cpp
        src/ImageService/ImageService.h
        src/ImageService/ImageService.cpp
        src/WebControlService/CmdExecute.cpp
        src/WebControlService/CmdExecute.h
        src/CommandService/SerialController.cpp
        src/CommandService/SerialController.h
        src/ConfigLoader/ConfigLoader.cpp
        src/ConfigLoader/ConfigLoader.h
        src/AsyncCallbackMailbox/AsyncCallbackMailbox.h
        src/CommandService/CmdSerialMail.h
        src/WebControlService/WebCmdMail.h
        src/ImageService/ImageServiceHttp.cpp
        src/ImageService/ImageServiceHttp.h
        src/ImageService/CameraReader.cpp
        src/ImageService/CameraReader.h
        src/ImageService/ImageServiceMail.h
        src/QueryPairsAnalyser/QueryPairsAnalyser.cpp
        src/QueryPairsAnalyser/QueryPairsAnalyser.h
        src/CommandService/CmdServiceHttp.cpp
        src/CommandService/CmdServiceHttp.h
        src/CommandService/ProcessJsonMessage.h)


target_link_libraries(${ProjectMainName} ${CMAKE_THREAD_LIBS_INIT})

include_directories(${Boost_INCLUDE_DIRS} ${PROTOBUFF_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${ProjectMainName} ${Boost_LIBRARIES} ${PROTOBUFF_LIBS} ${OpenCV_LIBRARIES})
link_directories(${Boost_LIBRARY_DIRS})


if (WIN32)
    target_link_libraries(${ProjectMainName} wsock32 ws2_32 crypt32)
else ()
endif ()
