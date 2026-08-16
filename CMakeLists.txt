cmake_minimum_required(VERSION 3.24)
project(
    md_fe_desktop
    VERSION 0.1.0
    DESCRIPTION "Desktop frontend for MitM medical devices"
    HOMEPAGE_URL "https://github.com/Zheng-Bote/md_fe_desktop"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find nlohmann_json (via Conan)
find_package(nlohmann_json REQUIRED)

# Find System Qt6
find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Fetch qtkeychain for secure token storage (avoids Conan Qt6 conflicts)
include(FetchContent)
FetchContent_Declare(
    qtkeychain
    GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
    GIT_TAG        087369387cbda8fbfbb9d70bd5e668319a8a8c4d
)
set(BUILD_TRANSLATIONS OFF CACHE BOOL "Disable translations for qtkeychain")
set(QTKEYCHAIN_STATIC ON CACHE BOOL "Build qtkeychain as a static library")
set(BUILD_WITH_QT6 ON CACHE BOOL "Build qtkeychain with Qt6")
set(BUILD_TESTING OFF CACHE BOOL "Disable tests" FORCE)
FetchContent_MakeAvailable(qtkeychain)

# Find Conan dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)

FetchContent_Declare(
  gh_update_checker
  GIT_REPOSITORY https://github.com/Zheng-Bote/gh-update-checker.git
  GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(gh_update_checker)
if(NOT TARGET gh_update_checker::gh_update_checker)
    add_library(gh_update_checker::gh_update_checker ALIAS gh_update_checker)
endif()

# --- Configure (für rz_config.hpp) ---
set(EXECUTABLE_NAME "${PROJECT_NAME}")
set(PROG_LONGNAME "${PROJECT_DESCRIPTION}")
set(PROG_CREATED "2026")
set(PROG_COPYRIGHT "ZHENG Robert")
set(PROG_LICENSE "Apache-2.0")
set(PROG_AUTHOR "ZHENG Bote")
set(PROG_ORGANIZATION_NAME "ZHENG Robert")
set(PROG_ORGANIZATION_DOMAIN "net.hase-zheng")
set(CMAKE_QTV "${Qt6_VERSION}")
# create rz_config.hpp
add_subdirectory(configure)

include_directories(include)

add_executable(md_fe_desktop
    src/main.cpp
    src/ConfigLoader.cpp
    include/ConfigLoader.hpp
    src/AuthService.cpp
    include/AuthService.hpp
    src/LoginWindow.cpp
    include/LoginWindow.hpp
    src/MainWindow.cpp
    include/MainWindow.hpp
    src/TokenManager.cpp
    include/TokenManager.hpp
    src/CryptoHelper.cpp
    include/CryptoHelper.hpp
)

target_link_libraries(md_fe_desktop PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
    openssl::openssl
    gh_update_checker::gh_update_checker
    qt6keychain
)

if (WIN32)
    # Target Windows SDK for WinRT (Windows Hello Biometrics)
    target_link_libraries(md_fe_desktop PRIVATE windowsapp)
endif()

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_libs.txt" 
     CONTENT "$<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>")