cmake_minimum_required(VERSION 3.16)
project(MapShortestPathVisualization VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(shortest_path_core
    src/graph.c
    src/dijkstra.c
    src/astar.c
    src/storage.c
    src/path_result.c
    src/input.c
    src/visualization.c
)

target_include_directories(shortest_path_core PUBLIC include)

if(NOT MSVC)
    target_compile_options(shortest_path_core PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(shortest_path_core PUBLIC m)
else()
    target_compile_options(shortest_path_core PRIVATE /W4)
endif()

add_executable(map_shortest_path src/main.c)
target_link_libraries(map_shortest_path PRIVATE shortest_path_core)
add_custom_command(TARGET map_shortest_path POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:map_shortest_path>/data"
    COMMENT "Copying map data next to map_shortest_path")

include(CTest)

option(MSP_ENABLE_3D "Build the optional raylib 3D renderer" OFF)
if(MSP_ENABLE_3D)
    find_package(raylib 5.0 QUIET)
    if(NOT raylib_FOUND)
        include(FetchContent)
        set(PLATFORM Desktop CACHE STRING "" FORCE)
        set(GRAPHICS_API_OPENGL_33 ON CACHE BOOL "" FORCE)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP OFF)
        FetchContent_MakeAvailable(raylib)
    endif()
    target_sources(map_shortest_path PRIVATE src/renderer3d.c src/road_editor.c src/ui_text.c
        src/renderer_phase1.c src/map_transform.c)
    target_link_libraries(map_shortest_path PRIVATE raylib)
    target_compile_definitions(map_shortest_path PRIVATE MSP_HAS_3D=1)
    if(BUILD_TESTING)
        add_executable(test_road_editor tests/test_road_editor.c
            src/road_editor.c src/map_transform.c)
        target_include_directories(test_road_editor PRIVATE include)
        target_link_libraries(test_road_editor PRIVATE raylib)
        add_test(NAME road_editor COMMAND test_road_editor)
    endif()
endif()

if(BUILD_TESTING)
    foreach(test_name graph dijkstra astar storage scu_map curved_campus input edge_cases path_result)
        add_executable(test_${test_name} tests/test_${test_name}.c)
        target_link_libraries(test_${test_name} PRIVATE shortest_path_core)
        add_test(NAME ${test_name} COMMAND test_${test_name})
    endforeach()
    target_compile_definitions(test_storage PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/test_data")
    target_compile_definitions(test_scu_map PRIVATE
        PROJECT_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
    target_compile_definitions(test_curved_campus PRIVATE
        PROJECT_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
endif()
