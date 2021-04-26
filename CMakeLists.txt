cmake_minimum_required(VERSION 3.10)
project(VulkanRenderer)

find_package(Vulkan REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main Qt5::Widgets)
