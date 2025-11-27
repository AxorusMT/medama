cmake_minimum_required(VERSION 3.26)
project(medama LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ----------------------------------------------------------
# Fetch wxWidgets source
# ----------------------------------------------------------
FetchContent_Declare(
    wxwidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG v3.2.6         # choose the version you prefer
)

# Configuration BEFORE adding the project
set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxUSE_WEBVIEW OFF CACHE BOOL "" FORCE)
set(wxUSE_STC OFF CACHE BOOL "" FORCE)
set(wxBUILD_CXX_STANDARD 20 CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(wxwidgets)

# ----------------------------------------------------------
# Create executable
# ----------------------------------------------------------
add_executable(medama-bin main.cpp)

if (WIN32)
    set_target_properties(medama-bin PROPERTIES
        WIN32_EXECUTABLE YES
    )
endif()


# Link the modules you need
target_link_libraries(medama-bin PRIVATE
    wx::core wx::base wx::adv wx::net wx::html
)
