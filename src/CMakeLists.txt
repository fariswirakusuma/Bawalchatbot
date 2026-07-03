cmake_minimum_required(VERSION 3.15)
project(ChatboxAI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Matikan fitur bawaan llama.cpp (WAJIB sebelum add_subdirectory)
set(LLAMA_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(LLAMA_BUILD_TESTS OFF CACHE INTERNAL "")
set(LLAMA_BUILD_SERVER OFF CACHE INTERNAL "")

# 2. Tambahkan subdirektori llama.cpp TERLEBIH DAHULU
add_subdirectory(third_party/llama.cpp)

# 3. Definisikan folder include proyek lokal kamu
include_directories(include)

# 4. Kumpulkan file source code lokal kamu
file(GLOB_RECURSE SOURCES "src/*.cpp" "main_cli.cpp")

# 5. Buat executable proyek kamu
add_executable(chatbox_core ${SOURCES})

# 6. Beritahu compiler lokasi header dari llama.cpp
target_include_directories(chatbox_core PRIVATE third_party/llama.cpp/include)

# 7. Hubungkan binary chatbox_core dengan library llama (Aktifkan link ini!)
target_link_libraries(chatbox_core PRIVATE llama)

# 8. Otomatisasi target untuk gateway agen AI
add_custom_target(gemini_debug
    COMMAND python3 ${CMAKE_SOURCE_DIR}/../scripts/agent_gateway.py --mode debugger --target $<TARGET_FILE:chatbox_core>
)