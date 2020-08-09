cmake_minimum_required(VERSION 3.13)
project(VK-nn VERSION 1.0 LANGUAGES C CXX)

set(RUNTIME_OUTPUT_NAME test.app)
set(CMAKE_BUILD_PARALLEL_LEVEL 12)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -O0 -g -Wall -Warray-bounds -Wdiv-by-zero -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=bounds -fsanitize=bounds-strict")

add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=16)
add_definitions(-DMAGICKCORE_HDRI_ENABLE=0)

# Clean
set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_BINARY_DIR}/bin/"
)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

# Main
add_executable(${RUNTIME_OUTPUT_NAME} main.cpp)

file(GLOB_RECURSE files 
  "Vulkan/*.cpp"
  "libs/*.cpp"
)
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
  opencv_core
  opencv_imgproc
  opencv_imgcodecs
  stdc++fs
)

#Shaders
file(GLOB_RECURSE files 
  "Shaders/test.comp"
)
foreach(file ${files})
  get_filename_component(FILE_NAME ${file} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/bin/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/bin/"
    COMMAND glslangValidator -V ${file} -o ${SPIRV}
    DEPENDS ${file})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(
  Shaders 
  DEPENDS ${SPIRV_BINARY_FILES}
  )

add_dependencies(${RUNTIME_OUTPUT_NAME} Shaders)

# Parallel
add_custom_target(Parallel)
add_custom_command(TARGET Parallel
  COMMAND make -j
)

# Run
add_custom_target(run DEPENDS Parallel)
add_custom_command(TARGET run
  COMMAND cd bin && ./${RUNTIME_OUTPUT_NAME}
)