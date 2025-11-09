cmake_minimum_required(VERSION 3.16)
project(OS-Simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(os_simulator ${SOURCES})
target_compile_definitions(os_simulator PRIVATE PROJECT_NAME="${PROJECT_NAME}")

if (MSVC)
    target_compile_options(os_simulator PRIVATE /W4 /permissive-)
else()
    target_compile_options(os_simulator PRIVATE -Wall -Wextra -pedantic)
endif()

# todo remove if unused
find_package(Threads REQUIRED)
target_link_libraries(os_simulator PRIVATE Threads::Threads)

add_custom_command(
    TARGET os_simulator PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/data/procesos
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/procesos
    COMMENT "Copying process data files to output dir"
)

option(BUILD_TESTS "Build test executables" ON)
if(BUILD_TESTS)
    Include(FetchContent)

    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.8.1 # or a later release
    )
    
    FetchContent_MakeAvailable(Catch2)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/tests/*.cpp
    )
    add_executable(tests ${TEST_SOURCES})
    target_compile_definitions(tests PRIVATE PROJECT_NAME="${PROJECT_NAME}")
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
    if (MSVC)
        target_compile_options(tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(tests PRIVATE -Wall -Wextra -pedantic)
    endif()
endif()
