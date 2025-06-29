project(map_tester)

cmake_minimum_required(VERSION 3.18)

include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

FetchContent_Declare(pntr URL https://github.com/RobLoach/pntr/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(pntr)

FetchContent_Declare(pntr_app URL https://github.com/RobLoach/pntr_app/archive/refs/heads/master.zip)
FetchContent_MakeAvailable(pntr_app)

FetchContent_Declare(raylib URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.zip)
FetchContent_MakeAvailable(raylib)

FILE(GLOB SRC_FILES src/*.c)
add_executable(${PROJECT_NAME} ${SRC_FILES})
ADD_COMPILE_DEFINITIONS(PNTR_APP_RAYLIB)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} pntr pntr_app raylib)


