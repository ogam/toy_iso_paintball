cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME toy_iso_paintball)

project(${PROJECT_NAME})

#set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(EMSCRIPTEN TRUE)
endif()

# Make sure all binaries are placed into the same build folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Link your project against CF statically, rather than as a shared library.
set(CUTE_FRAMEWORK_STATIC ON)
set(CF_FRAMEWORK_BUILD_SAMPLES OFF)

# This will download and build Cute Framework just once the first time you build your game.
include(FetchContent)
FetchContent_Declare(
	cute
#	original repo
	GIT_REPOSITORY https://github.com/RandyGaul/cute_framework
	GIT_TAG 94c078594e301d474017d47f9165faaebd10bce8
#	SOURCE_DIR "../../cute_framework"
)
FetchContent_MakeAvailable(cute)

# Any additional paths needed should either be under /src or add a new one for any additional paths that's not under /src
file(GLOB_RECURSE PROJECT_SOURCE_C
     "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
)

# Source code for your game.
add_executable(
	${PROJECT_NAME}
	${PROJECT_SOURCE_C}
)

# Our source code will be in the `src` folder.
target_include_directories(${PROJECT_NAME} PUBLIC 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libs/empyreanx>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libs/nicbarker>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libs/nothings>
)

# Support for web builds through the Emscripten compiler.
if(EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	target_compile_options(${PROJECT_NAME} PUBLIC -O1 -fno-rtti -fno-exceptions)
	target_link_options(${PROJECT_NAME} PRIVATE -o ${PROJECT_NAME}.html --preload-file ${CMAKE_SOURCE_DIR}/content@/content --emrun -s ASYNCIFY=1 -O1)
endif()

# Some basic information needed for CMake to generate your Info.plist file.
# This is necessary for e.g. iOS builds.
if(APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.${PROJECT_NAME}.${PROJECT_NAME}"
		MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
	)
endif()

# Make the game link against Cute Framework.
target_link_libraries(${PROJECT_NAME} cute)

# For convenience on Windows, set MSVC debugger's working directory in the build folder.
# Also ask MSVC to make the game the startup project.
if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

add_link_options(
    /LARGEADDRESSAWARE
    /DEBUG
    $<$<NOT:$<CONFIG:DEBUG>>:/INCREMENTAL:NO> # Disable incremental linking.
)
