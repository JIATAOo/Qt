# OBS Studio third-party bundle
# Located alongside the reference project
set(OBS_ROOT_DIR "G:/meeting-client-main/meeting-client-main/capture/third_party/obs")

if(NOT EXISTS "${OBS_ROOT_DIR}")
    message(WARNING "OBS SDK not found at ${OBS_ROOT_DIR}, trying system paths...")
    # Fallback: try vcpkg or system installation
    find_path(OBS_INCLUDE_DIRS obs.h
        PATHS
            "${OBS_ROOT_DIR}/include"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/include/obs"
        NO_DEFAULT_PATH
    )
    if(NOT OBS_INCLUDE_DIRS)
        message(FATAL_ERROR "OBS SDK not found. Please install OBS Studio SDK.")
    endif()
else()
    set(OBS_INCLUDE_DIRS ${OBS_ROOT_DIR}/include)
endif()

if(MSVC)
    # Use release OBS DLLs even in Debug to avoid CRT mismatch
    set(OBS_RUNTIME_DIR ${OBS_ROOT_DIR}/win_msvc_x64_release)
    set(OBS_LIB_DIR ${OBS_RUNTIME_DIR}/bin)
    set(OBS_PLUGIN_DIR ${OBS_RUNTIME_DIR}/obs-plugins)
    set(OBS_DATA_DIR ${OBS_ROOT_DIR}/data)
    set(OBS_DEPS_DIR ${OBS_ROOT_DIR}/deps/win_x64)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${OBS_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${OBS_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE obs)

# Post-build: copy OBS DLLs, plugins, and data to output directory
if(WIN32 AND MSVC)
    # zlib — OBS requires the DLL; vcpkg default build is static, so use reference project's
    set(ZLIB_DLL_SRC "G:/meeting-client-main/meeting-client-main/build/x64-debug/rundir/zlib1.dll")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # OBS core DLLs
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_LIB_DIR}/obs.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_LIB_DIR}/libobs-d3d11.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_LIB_DIR}/libobs-opengl.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_LIB_DIR}/w32-pthreads.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        # zlib (required by OBS, from vcpkg)
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ZLIB_DLL_SRC}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/zlib.dll"
        # FFmpeg dependencies
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/avcodec-61.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/avformat-61.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/avutil-59.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/swresample-5.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/swscale-8.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/libx264-164.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/srt.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OBS_DEPS_DIR}/librist.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        # OBS plugins
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OBS_PLUGIN_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/obs-plugins/"
        # OBS data (shaders, locale)
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OBS_DATA_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data/"
        COMMENT "Copying OBS runtime files..."
    )
endif()
