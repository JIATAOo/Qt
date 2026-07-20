set(LIBSAMPLERATE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libsamplerate)

set(LIBSAMPLERATE_INCLUDE_DIRS ${LIBSAMPLERATE_ROOT_DIR}/include)
set(LIBSAMPLERATE_LIBRARIES samplerate)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        set(LIBSAMPLERATE_DIRECTORY ${LIBSAMPLERATE_ROOT_DIR}/win_x64_debug)
    else()
        set(LIBSAMPLERATE_DIRECTORY ${LIBSAMPLERATE_ROOT_DIR}/win_x64_release)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBSAMPLERATE_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBSAMPLERATE_DIRECTORY})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBSAMPLERATE_LIBRARIES})

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIBSAMPLERATE_DIRECTORY}/samplerate.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying samplerate.dll..."
    )
endif()
