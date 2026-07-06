cmake_minimum_required(VERSION 3.15)
project(ChatboxAI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LLAMA_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(LLAMA_BUILD_TESTS OFF CACHE INTERNAL "")
set(LLAMA_BUILD_SERVER OFF CACHE INTERNAL "")

add_subdirectory(../third_party/llama.cpp llama_build)

find_package(CURL REQUIRED)

file(GLOB_RECURSE APP_SOURCES "src/*.cpp")
list(FILTER APP_SOURCES EXCLUDE REGEX ".*test.*\\.cpp")

add_executable(chatbox_core main.cpp ${APP_SOURCES})

target_include_directories(chatbox_core PRIVATE 
    include
    ../third_party/llama.cpp/include
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(chatbox_core PRIVATE llama ${CURL_LIBRARIES})

include(FetchContent) 

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp") 

if(TEST_SOURCES)
    add_executable(core_test_runner ${TEST_SOURCES} ${APP_SOURCES})
    
    target_include_directories(core_test_runner PRIVATE 
        include
        ../third_party/llama.cpp/include
        ${CURL_INCLUDE_DIRS}
    )
    
    target_link_libraries(core_test_runner PRIVATE 
        llama 
        ${CURL_LIBRARIES} 
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(core_test_runner)
endif()

add_custom_target(gemini_debug
    COMMAND python3 ${CMAKE_SOURCE_DIR}/../scripts/agent_gateway.py --mode debugger --target $<TARGET_FILE:chatbox_core>
)