set(LIBDATACHANNEL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libdatachannel)

set(LIBDATACHANNEL_INCLUDE_DIRS ${LIBDATACHANNEL_ROOT_DIR}/include)
set(LIBDATACHANNEL_LIBRARIES datachannel juice srtp2 usrsctp libssl libcrypto)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        set(LIBDATACHANNEL_DIRECTORY ${LIBDATACHANNEL_ROOT_DIR}/win_x64_debug)
    else()
        set(LIBDATACHANNEL_DIRECTORY ${LIBDATACHANNEL_ROOT_DIR}/win_x64_release)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBDATACHANNEL_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBDATACHANNEL_DIRECTORY})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBDATACHANNEL_LIBRARIES})

if(WIN32)
    # Copy all required DLLs
    foreach(DLL_NAME datachannel juice srtp2 usrsctp libssl-3-x64 libcrypto-3-x64 legacy)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIBDATACHANNEL_DIRECTORY}/${DLL_NAME}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
            COMMENT "Copying ${DLL_NAME}.dll..."
        )
    endforeach()
endif()
