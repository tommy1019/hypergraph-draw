cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(hypergraph VERSION 1.0 LANGUAGES CXX)

find_package(PkgConfig)
pkg_check_modules(GRAPHVIZ REQUIRED libgvc libcgraph)
pkg_check_modules(JSON REQUIRED nlohmann_json)

add_executable(hypergraph-draw hypergraph-draw.cpp)
target_link_directories(hypergraph-draw PUBLIC ${JSON_LIBRARY_DIRS})
target_link_libraries(hypergraph-draw PUBLIC ${JSON_LIBRARIES})
target_include_directories(hypergraph-draw PUBLIC ${JSON_INCLUDE_DIRS})

if (GRAPHVIZ_FOUND)
add_executable(hypergraph-layout hypergraph-layout.cpp)
target_link_directories(hypergraph-layout PUBLIC ${GRAPHVIZ_LIBRARY_DIRS} ${JSON_LIBRARY_DIRS})
target_link_libraries(hypergraph-layout PUBLIC ${GRAPHVIZ_LIBRARIES} ${JSON_LIBRARIES})
target_include_directories(hypergraph-layout PUBLIC ${GRAPHVIZ_INCLUDE_DIRS} ${JSON_INCLUDE_DIRS})
endif()