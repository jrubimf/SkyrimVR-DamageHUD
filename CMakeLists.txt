cmake_minimum_required(VERSION 3.21)

project(
	FloatingDamage
	VERSION 1.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CommonLibVR
set(BUILD_SKYRIMVR ON CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)

if(DEFINED ENV{CommonLibVRPath})
	add_subdirectory("$ENV{CommonLibVRPath}" CommonLibVR)
else()
	add_subdirectory(extern/CommonLibVR)
endif()

file(GLOB_RECURSE SOURCES
	"src/*.cpp"
	"src/*.h"
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_definitions(${PROJECT_NAME} PRIVATE SKYRIMVR)

find_library(OPENVR_LIB openvr_api REQUIRED)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		CommonLibVR::CommonLibVR
		${OPENVR_LIB}
)

target_precompile_headers(${PROJECT_NAME}
	PRIVATE
		src/PCH.h
)

if(MSVC)
	target_compile_options(${PROJECT_NAME}
		PRIVATE
			/permissive-
			/Zc:preprocessor
			/sdl
			/utf-8
			/Zi
			/W4
			/WX-
			/wd4100  # unreferenced formal parameter
			/wd4189  # local variable is initialized but not referenced
	)
endif()

# Optional: Copy to Skyrim VR mods folder
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
	set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
	set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
		VERBATIM
	)
endif()
