cmake_minimum_required(VERSION 3.15)
project(ChatboxAI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp" "main.cpp")

add_executable(chatbox_core ${SOURCES})

# target_link_libraries(chatbox_core PRIVATE llama)

add_custom_target(gemini_debug
    COMMAND python3 ${CMAKE_SOURCE_DIR}/../scripts/agent_gateway.py --mode debugger --target $<TARGET_FILE:chatbox_core>
)