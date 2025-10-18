# FetchVulkan.cmake
# Module to fetch and setup Vulkan SDK if not found on the system
#
# Usage:
#   include(modules/FetchVulkan.cmake)
#
# This will set the following variables:
#   VULKAN_FOUND - TRUE if Vulkan was found or fetched successfully
#   VULKAN_INCLUDE_DIRS - Path to Vulkan include directories
#   VULKAN_LIBRARIES - Path to Vulkan libraries
#   VULKAN_VERSION - Version of Vulkan SDK

include(FetchContent)

# First, try to find Vulkan on the system
find_package(Vulkan QUIET)

if(Vulkan_FOUND)
    message(STATUS "Found Vulkan SDK ${Vulkan_VERSION} at ${Vulkan_INCLUDE_DIRS}")
    set(VULKAN_FOUND TRUE)
    set(VULKAN_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
    set(VULKAN_LIBRARIES ${Vulkan_LIBRARIES})
    set(VULKAN_VERSION ${Vulkan_VERSION})
else()
    message(STATUS "Vulkan SDK not found on system, attempting to fetch...")
    
    set(VULKAN_FOUND FALSE)
    set(VULKAN_SDK_VERSION "1.3.280.0")
    
    if(WIN32)
        # For Windows, we'll fetch the Vulkan SDK installer
        # Note: This downloads the installer but doesn't run it automatically
        # Users should install it manually or we can provide instructions
        
        message(STATUS "Fetching Vulkan SDK ${VULKAN_SDK_VERSION} for Windows...")
        message(WARNING "Automatic Vulkan SDK installation requires administrator privileges.")
        message(WARNING "Please download and install Vulkan SDK from: https://vulkan.lunarg.com/")
        
        # Alternative: Fetch Vulkan headers and use system's Vulkan loader
        message(STATUS "Fetching Vulkan headers as fallback...")
        
        FetchContent_Declare(
            vulkan_headers
            GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
            GIT_TAG v1.3.280
            GIT_SHALLOW TRUE
        )
        
        FetchContent_MakeAvailable(vulkan_headers)
        
        # Try to find vulkan-1.lib in common locations
        find_library(VULKAN_LIBRARY
            NAMES vulkan-1 vulkan
            PATHS
                "$ENV{VULKAN_SDK}/Lib"
                "$ENV{VK_SDK_PATH}/Lib"
                "C:/VulkanSDK/*/Lib"
            NO_DEFAULT_PATH
        )
        
        if(VULKAN_LIBRARY)
            set(VULKAN_FOUND TRUE)
            set(VULKAN_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
            set(VULKAN_LIBRARIES ${VULKAN_LIBRARY})
            message(STATUS "Using Vulkan headers from: ${VULKAN_INCLUDE_DIRS}")
            message(STATUS "Using Vulkan library from: ${VULKAN_LIBRARY}")
        else()
            # Still provide headers for development
            set(VULKAN_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
            message(WARNING "Vulkan library not found. Headers available but linking will fail.")
            message(WARNING "Please install Vulkan SDK from: https://vulkan.lunarg.com/")
        endif()
        
    elseif(UNIX AND NOT APPLE)
        # For Linux, fetch Vulkan SDK or headers
        message(STATUS "Fetching Vulkan headers for Linux...")
        
        FetchContent_Declare(
            vulkan_headers
            GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
            GIT_TAG v1.3.280
            GIT_SHALLOW TRUE
        )
        
        FetchContent_MakeAvailable(vulkan_headers)
        
        # Try to find system Vulkan library
        find_library(VULKAN_LIBRARY
            NAMES vulkan
            PATHS
                /usr/lib
                /usr/lib/x86_64-linux-gnu
                /usr/local/lib
            NO_DEFAULT_PATH
        )
        
        if(VULKAN_LIBRARY)
            set(VULKAN_FOUND TRUE)
            set(VULKAN_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
            set(VULKAN_LIBRARIES ${VULKAN_LIBRARY})
            message(STATUS "Using Vulkan headers from: ${VULKAN_INCLUDE_DIRS}")
            message(STATUS "Using Vulkan library from: ${VULKAN_LIBRARY}")
        else()
            set(VULKAN_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
            message(WARNING "Vulkan library not found. Headers available but linking will fail.")
            message(WARNING "Install Vulkan: sudo apt-get install libvulkan-dev vulkan-tools")
        endif()
        
    elseif(APPLE)
        # For macOS, fetch Vulkan SDK (MoltenVK)
        message(STATUS "Fetching Vulkan SDK (MoltenVK) for macOS...")
        
        FetchContent_Declare(
            vulkan_sdk_macos
            URL "https://sdk.lunarg.com/sdk/download/1.3.280.0/mac/vulkansdk-macos-1.3.280.0.dmg"
            DOWNLOAD_NO_EXTRACT TRUE
        )
        
        # Also fetch headers
        FetchContent_Declare(
            vulkan_headers
            GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
            GIT_TAG v1.3.280
            GIT_SHALLOW TRUE
        )
        
        FetchContent_MakeAvailable(vulkan_headers)
        
        set(VULKAN_INCLUDE_DIRS "${vulkan_headers_SOURCE_DIR}/include")
        
        # Try to find MoltenVK framework
        find_library(MOLTENVK_LIBRARY
            NAMES MoltenVK
            PATHS
                "$ENV{VULKAN_SDK}/lib"
                "$ENV{VULKAN_SDK}/MoltenVK/dylib/macOS"
                "/usr/local/lib"
            NO_DEFAULT_PATH
        )
        
        if(MOLTENVK_LIBRARY)
            set(VULKAN_FOUND TRUE)
            set(VULKAN_LIBRARIES ${MOLTENVK_LIBRARY})
            message(STATUS "Using Vulkan headers from: ${VULKAN_INCLUDE_DIRS}")
            message(STATUS "Using MoltenVK from: ${MOLTENVK_LIBRARY}")
        else()
            message(WARNING "MoltenVK library not found. Headers available but linking will fail.")
            message(WARNING "Please install Vulkan SDK from: https://vulkan.lunarg.com/sdk/home#mac")
        endif()
    else()
        message(WARNING "Unsupported platform for automatic Vulkan fetch")
    endif()
    
    if(NOT VULKAN_FOUND)
        message(WARNING "Vulkan SDK could not be found or fetched completely.")
        message(WARNING "Please install Vulkan SDK manually from: https://vulkan.lunarg.com/")
    endif()
endif()

# Create an interface library for easy linking
if(VULKAN_FOUND AND NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan INTERFACE IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${VULKAN_LIBRARIES}"
    )
endif()

# Export variables for parent scope
set(VULKAN_FOUND ${VULKAN_FOUND} PARENT_SCOPE)
set(VULKAN_INCLUDE_DIRS ${VULKAN_INCLUDE_DIRS} PARENT_SCOPE)
set(VULKAN_LIBRARIES ${VULKAN_LIBRARIES} PARENT_SCOPE)
set(VULKAN_VERSION ${VULKAN_SDK_VERSION} PARENT_SCOPE)
