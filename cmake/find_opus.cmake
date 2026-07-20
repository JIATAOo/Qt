set(OPUS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/opus)
set(OPUS_INCLUDE_DIRS ${OPUS_ROOT_DIR}/include)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        set(OPUS_LIB_DIR ${OPUS_ROOT_DIR}/win_x64_debug)
    else()
        set(OPUS_LIB_DIR ${OPUS_ROOT_DIR}/win_x64_release)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${OPUS_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${OPUS_LIB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE opus)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPUS_LIB_DIR}/opus.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying opus.dll..."
    )
endif()
