cmake_minimum_required(VERSION 3.20)
project(toi-cplus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies via vcpkg
set(VPKG_DIR "/home/joshh/Documents/NLT/Engine/Plugins/Runtime/GeoReferencing/Source/ThirdParty/vcpkg-installed/overlay-x64-uwp")
list(APPEND CMAKE_PREFIX_PATH "/home/joshh/Documents/NLT/Engine/Plugins/Runtime/GeoReferencing/Source/ThirdParty/vcpkg-installed/overlay-x64-uwp")
find_package(nlohmann_json 3.4.0 REQUIRED)
find_package(Catch2 3.4.0 REQUIRED)
find_package(spdlog 1.11.0 REQUIRED)
find_package(tl_expected 2.0.0 REQUIRED)

# TOI library
add_library(toi-cplus
    include/toi/TOITypes.h
    include/toi/TermsOfInteraction.h
    src/TermsOfInteraction.cpp
    src/TOIManager.cpp
)

target_name(toi-cplus)
set_target_properties(toi-cplus PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF)

target_link_libraries(toi-cplus PRIVATE
    nlohmann_json::nlohmann_json
    Catch2::Catch2
    spdlog::spdlog
    tl_expected::tl_expected)

# Include directories
target_include_directories(toi-cplus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Tests
enable_testing()
add_test(TOI test_toi_manager)