cmake_minimum_required(VERSION 3.20)
project(MusicFeed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- nlohmann/json (header-only) pulled automatically at configure time ---
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        # ---GIT_REPOSITORY https://github.com/nlohmann/json.git
        # ---GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- libcurl (system library, ships with macOS) for the HTTPS request ---
find_package(CURL REQUIRED)

add_executable(MusicFeed main.cpp)
target_link_libraries(MusicFeed PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
