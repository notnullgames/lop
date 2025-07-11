cmake_minimum_required(VERSION 3.18)
project(lop)

include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

# set(CMAKE_BUILD_TYPE Debug)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode.")
    add_definitions(-DDEBUG_MODE)
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
endif()

FetchContent_Declare(pntr URL https://github.com/RobLoach/pntr/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(pntr)

FetchContent_Declare(pntr_app URL https://github.com/RobLoach/pntr_app/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(pntr_app)

FetchContent_Declare(pntr_tiled URL https://github.com/RobLoach/pntr_tiled/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(pntr_tiled)

FetchContent_Declare(pntr_app_sfx URL https://github.com/konsumer/pntr_app_sfx/archive/refs/heads/main.zip)
FetchContent_MakeAvailable(pntr_app_sfx)

FILE(GLOB SRC_FILES src/*.c)
add_executable(${PROJECT_NAME} ${SRC_FILES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} pntr pntr_app pntr_tiled)
target_include_directories(${PROJECT_NAME} PUBLIC ${pntr_app_sfx_SOURCE_DIR})


IF (EMSCRIPTEN)
  ADD_COMPILE_DEFINITIONS(PNTR_APP_WEB)
  SET(CMAKE_EXECUTABLE_SUFFIX ".mjs")
  TARGET_LINK_OPTIONS(${PROJECT_NAME} PRIVATE -sASSERTIONS=1 -sASYNCIFY --embed-file ${CMAKE_SOURCE_DIR}/assets@/assets)
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.wasm" "${CMAKE_SOURCE_DIR}/docs/${PROJECT_NAME}.wasm"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.mjs" "${CMAKE_SOURCE_DIR}/docs/${PROJECT_NAME}.mjs"
  )
ELSE()
  ADD_COMPILE_DEFINITIONS(PNTR_APP_RAYLIB)
  FetchContent_Declare(raylib URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip)
  FetchContent_MakeAvailable(raylib)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} raylib)
ENDIF()