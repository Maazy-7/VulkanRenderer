cmake_minimum_required(VERSION 3.16)
project(VulkanRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/VulkanRenderer)

set(SRC_FILES 
	${SRC_ROOT}/main.cpp
	${SRC_ROOT}/Application.cpp
	${SRC_ROOT}/InputManager.cpp
	${SRC_ROOT}/VulkanDevice.cpp
	${SRC_ROOT}/VulkanPipeline.cpp
	${SRC_ROOT}/VulkanSwapChain.cpp
	${SRC_ROOT}/Window.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
${SRC_ROOT}
${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Include)

target_link_libraries(${PROJECT_NAME} PRIVATE
Vulkan::Vulkan
glfw
glm::glm)