cmake_minimum_required (VERSION 3.31)

project ("FatRenderer")

# set the language standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# finding the Vulkan SDK
find_package(Vulkan REQUIRED)

# add all executables
file (GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
file (GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "src/*.h" "src/*.hpp")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

# include additional files
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# set up precompiled Vulkan library
# create an empty pchLibrary
add_library(VulkanCppPCH INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppPCH)

# set up necessary macros
target_compile_definitions(VulkanCppPCH INTERFACE
	"VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1"
	"VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1"
)

# set up parent Vulkan module and directory for headers
target_include_directories(VulkanCppPCH INTERFACE "${Vulkan_INCLUDE_DIR}")
target_link_libraries(VulkanCppPCH INTERFACE Vulkan::Vulkan)

target_precompile_headers(VulkanCppPCH INTERFACE
	<vulkan/vulkan.hpp>
)

# set up a path for libraries and include them
target_link_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
target_link_libraries(${PROJECT_NAME} PRIVATE 
	glfw3
	Vulkan::cppm)