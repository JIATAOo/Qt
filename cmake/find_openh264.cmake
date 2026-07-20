set(OPENH264_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openh264)

set(OPENH264_INCLUDE_DIRS ${OPENH264_ROOT_DIR}/include)
set(OPENH264_LIBRARIES openh264)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        set(OPENH264_DIRECTORY ${OPENH264_ROOT_DIR}/win_x64_debug)
    else()
        set(OPENH264_DIRECTORY ${OPENH264_ROOT_DIR}/win_x64_release)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${OPENH264_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${OPENH264_DIRECTORY})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENH264_LIBRARIES})

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENH264_DIRECTORY}/openh264-7.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying openh264-7.dll..."
    )
endif()
