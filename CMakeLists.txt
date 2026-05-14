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

# find slangc compiler for shaders and and our shaders for compiling
find_program(SLANGC_EXECUTABLE slangc REQUIRED)
set(SHADER_SLANG_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang
)



# create function for compiling slang shaders
function (add_slang_shader_target TARGET)
	# parse list of shader sources and assign them ${SHADER_SOURCES} macro
	cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
	# set the output directory, entry point for compiled shaders
	set(SHADERS_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
	set(ENTRY_POINTS -entry vertMain -entry fragMain)
	set(OUTPUT_SPV ${SHADERS_OUT_DIR}/slang.spv)

	# make output directory and compile shaders
	file(MAKE_DIRECTORY ${SHADERS_OUT_DIR})
	add_custom_command(
		OUTPUT ${OUTPUT_SPV}
		COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${OUTPUT_SPV}
		DEPENDS ${SHADER_SOURCES}
		COMMENT "Compiling Slang Shaders"
		VERBATIM
	)
	add_custom_target(${TARGET} DEPENDS ${OUTPUT_SPV})
endfunction()

add_slang_shader_target(shader_compiling SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(FatRenderer shader_compiling)

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

target_link_libraries(${PROJECT_NAME} PRIVATE 
	glfw3
	Vulkan::cppm)

target_precompile_headers(VulkanCppPCH INTERFACE
	<vulkan/vulkan.hpp>
)

# set up a path for libraries and include them
target_link_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

