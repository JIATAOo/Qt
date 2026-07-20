# find_spdlog.cmake - spdlog with external fmt from reference project
set(SPDLOG_INCLUDE_DIR "G:/meeting-client-main/meeting-client-main/logger/third_party/spdlog/include")
set(FMT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog)

target_include_directories(${PROJECT_NAME} PRIVATE ${SPDLOG_INCLUDE_DIR})

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        set(FMT_LIB_DIR ${FMT_DIR}/win_x64_debug)
        set(FMT_LIB fmtd)
    else()
        set(FMT_LIB_DIR ${FMT_DIR}/win_x64_release)
        set(FMT_LIB fmt)
    endif()
endif()

target_link_directories(${PROJECT_NAME} PRIVATE ${FMT_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${FMT_LIB})

# Copy fmt DLL post-build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FMT_LIB_DIR}/${FMT_LIB}.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
    COMMENT "Copying ${FMT_LIB}.dll..."
)
