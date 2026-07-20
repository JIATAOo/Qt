cmake_minimum_required(VERSION 3.21)
project(MyVideoConference VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 让 Qt 的 CMake 工具自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Network)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/engine/Engine.cpp
    src/engine/Engine.h
    src/capture/CaptureTypes.h
    src/capture/CaptureEngine.cpp
    src/capture/CaptureEngine.h
    src/logger/LogLevel.h
    src/logger/Logger.h
    src/logger/Logger.cpp
    src/widgets/LoginDlg.cpp
    src/widgets/LoginDlg.h
    src/widgets/MainWindow.cpp
    src/widgets/MainWindow.h
    src/widgets/CreateMeetingDialog.cpp
    src/widgets/CreateMeetingDialog.h
    src/widgets/JoinMeetingDialog.cpp
    src/widgets/JoinMeetingDialog.h
    src/widgets/MeetingRoom.cpp
    src/widgets/MeetingRoom.h
    src/transport/TransportDefine.h
    src/transport/ITransportEngine.h
    src/transport/RtcTransportBase.cpp
    src/transport/RtcTransportBase.h
    src/transport/RtcPushTransport.cpp
    src/transport/RtcPushTransport.h
    src/transport/RtcPullTransport.cpp
    src/transport/RtcPullTransport.h
    src/transport/WHIPWHEPClient.cpp
    src/transport/WHIPWHEPClient.h
    src/transport/TransportEngine.cpp
    src/transport/TransportEngine.h
    src/transport/codec/video_codec/VideoCodecInterface.h
    src/transport/codec/video_codec/VideoCodecFactory.cpp
    src/transport/codec/video_codec/VideoCodecFactory.h
    src/transport/codec/video_codec/H264Codec.cpp
    src/transport/codec/video_codec/H264Codec.h
    src/transport/codec/audio_codec/AudioCodecInterface.h
    src/transport/codec/audio_codec/AudioCodecFactory.cpp
    src/transport/codec/audio_codec/AudioCodecFactory.h
    src/transport/codec/audio_codec/OpusCodec.cpp
    src/transport/codec/audio_codec/OpusCodec.h
    ui/LoginDlg.ui
    ui/MainWindow.ui
    ui/CreateMeetingDialog.ui
    ui/JoinMeetingDialog.ui
    ui/MeetingRoom.ui
    res/resource.qrc
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/capture
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/codec/video_codec
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport/codec/audio_codec
)

# OBS Studio SDK
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_obs.cmake)

# spdlog (header-only, from reference project)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_spdlog.cmake)

# OpenH264 (H.264 video codec)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_openh264.cmake)

# Opus (audio codec)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_opus.cmake)

# libsamplerate (audio resampling)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_libsamplerate.cmake)

# libdatachannel (WebRTC)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_libdatachannel.cmake)

# Post-build: copy Qt DLLs from vcpkg
if(WIN32 AND MSVC)
    if(DEFINED VCPKG_INSTALLED_DIR)
        set(_VCPKG_BIN "${VCPKG_INSTALLED_DIR}/bin")
        set(_VCPKG_DBG "${VCPKG_INSTALLED_DIR}/debug/bin")
    else()
        set(_VCPKG_BIN "$ENV{VCPKG_ROOT}/installed/x64-windows/bin")
        set(_VCPKG_DBG "$ENV{VCPKG_ROOT}/installed/x64-windows/debug/bin")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${_VCPKG_DBG},${_VCPKG_BIN}>/Qt6Cored$<$<NOT:$<CONFIG:Debug>>.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${_VCPKG_DBG},${_VCPKG_BIN}>/Qt6Widgetsd$<$<NOT:$<CONFIG:Debug>>.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${_VCPKG_DBG},${_VCPKG_BIN}>/Qt6Multimediad$<$<NOT:$<CONFIG:Debug>>.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${_VCPKG_DBG},${_VCPKG_BIN}>/Qt6Networkd$<$<NOT:$<CONFIG:Debug>>.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying Qt runtime DLLs..."
    )
endif()
