cmake_minimum_required(VERSION 3.13)
set(PROJECT_NAME VK-nn)
set(RUNTIME_OUTPUT_NAME test.app)

project(${PROJECT_NAME} VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O0 -g -Wall -Warray-bounds -Wdiv-by-zero -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=bounds -fsanitize=bounds-strict")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenMP REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(${RUNTIME_OUTPUT_NAME} main.cpp)

file(GLOB files "Vulkan/*.cpp")
foreach(file ${files})
  target_sources(${RUNTIME_OUTPUT_NAME} PRIVATE ${file})
endforeach()

file(GLOB files "libs/*.cpp")
foreach(file ${files})
  target_sources(${RUNTIME_OUTPUT_NAME} PRIVATE ${file})
endforeach()

set_target_properties(${RUNTIME_OUTPUT_NAME} PROPERTIES 
 CXX_STANDARD 17
 CXX_STANDARD_REQUIRED ON
)

set_target_properties(${RUNTIME_OUTPUT_NAME}
 PROPERTIES
 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(${RUNTIME_OUTPUT_NAME}
 glfw
 ${OpenMP_LIBRARIES}
 ${Vulkan_LIBRARIES}
 ${glfw3_LIBRARIES}
 ${OpenCV_LIBRARIES}
)
