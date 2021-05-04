cmake_minimum_required(VERSION 3.10)
project(VulkanRenderer)

#function for building sprv shaders
function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER})
    set(current-output-path ${CMAKE_BINARY_DIR}/shaders/${SHADER}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
           OUTPUT ${current-output-path}
           COMMAND ${GLSLC} -o ${current-output-path} ${current-shader-path}
           DEPENDS ${current-shader-path}
           IMPLICIT_DEPENDS CXX ${current-shader-path}
           VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

set(CMAKE_CXX_STANDARD 23)

#setup conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

#handle locating external dependencies
find_package(Vulkan REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

#source files for target
set(SOURCES src/main.cpp)
add_executable(main ${SOURCES})

#shaders to build
add_shader(main triangle.vert)
add_shader(main triangle.frag)

#link in external dependencies
target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main Qt5::Widgets)
