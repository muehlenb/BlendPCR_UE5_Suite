cmake_minimum_required(VERSION 3.20)
project(PoseSolver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# vcpkg Packages
# -------------------------------------------------------
find_package(Eigen3 CONFIG REQUIRED)
find_package(g2o CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(PoseSolver
    src/main.cpp
)

# -------------------------------------------------------
# Includes (ASIO header-only)
# -------------------------------------------------------
target_include_directories(PoseSolver PRIVATE
    ${ASIO_INCLUDE_DIR}
)

# -------------------------------------------------------
# Link dependencies
# -------------------------------------------------------
target_link_libraries(PoseSolver PRIVATE
    Eigen3::Eigen
    g2o::core
    g2o::types_slam3d
    g2o::stuff
    nlohmann_json::nlohmann_json
)

# -------------------------------------------------------
# Compiler Definitions
# -------------------------------------------------------
target_compile_definitions(PoseSolver PRIVATE ASIO_STANDALONE)

# Windows needs WinSock
if (WIN32)
    target_link_libraries(PoseSolver PRIVATE ws2_32)
endif()
