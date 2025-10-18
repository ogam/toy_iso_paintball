# FetchPython.cmake
# Module to fetch and setup Python if not found on the system
#
# Usage:
#   include(modules/FetchPython.cmake)
#
# This will set the following variables:
#   PYTHON_FOUND - TRUE if Python was found or fetched successfully
#   PYTHON_EXECUTABLE - Path to the Python executable
#   PYTHON_VERSION - Version of Python found

include(FetchContent)

# First, try to find Python on the system
find_package(Python3 3.8 QUIET COMPONENTS Interpreter Development)

if(Python3_FOUND)
    message(STATUS "Found Python ${Python3_VERSION} at ${Python3_EXECUTABLE}")
    set(PYTHON_FOUND TRUE)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    set(PYTHON_VERSION ${Python3_VERSION})
else()
    message(STATUS "Python3 not found on system, attempting to fetch...")
    
    set(PYTHON_FOUND FALSE)
    set(PYTHON_VERSION "3.12.0")
    
    if(WIN32)
        # Determine architecture
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(PYTHON_ARCH "amd64")
            set(PYTHON_URL "https://www.python.org/ftp/python/${PYTHON_VERSION}/python-${PYTHON_VERSION}-embed-${PYTHON_ARCH}.zip")
        else()
            set(PYTHON_ARCH "win32")
            set(PYTHON_URL "https://www.python.org/ftp/python/${PYTHON_VERSION}/python-${PYTHON_VERSION}-embed-${PYTHON_ARCH}.zip")
        endif()
        
        message(STATUS "Fetching Python ${PYTHON_VERSION} embeddable package for Windows (${PYTHON_ARCH})...")
        
        FetchContent_Declare(
            python_embed
            URL ${PYTHON_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        
        FetchContent_MakeAvailable(python_embed)
        
        # Set Python executable path
        set(PYTHON_EXECUTABLE "${python_embed_SOURCE_DIR}/python.exe")
        
        if(EXISTS "${PYTHON_EXECUTABLE}")
            set(PYTHON_FOUND TRUE)
            message(STATUS "Python embedded package downloaded to: ${python_embed_SOURCE_DIR}")
            message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")
            
            # Add Python to PATH for the build
            set(ENV{PATH} "${python_embed_SOURCE_DIR};$ENV{PATH}")
        else()
            message(WARNING "Failed to setup Python embeddable package")
        endif()
        
    elseif(UNIX AND NOT APPLE)
        # For Linux, download standalone Python build
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
            set(PYTHON_URL "https://github.com/indygreg/python-build-standalone/releases/download/20231002/cpython-3.12.0+20231002-x86_64-unknown-linux-gnu-install_only.tar.gz")
            
            message(STATUS "Fetching Python ${PYTHON_VERSION} standalone build for Linux...")
            
            FetchContent_Declare(
                python_standalone
                URL ${PYTHON_URL}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            
            FetchContent_MakeAvailable(python_standalone)
            
            set(PYTHON_EXECUTABLE "${python_standalone_SOURCE_DIR}/bin/python3")
            
            if(EXISTS "${PYTHON_EXECUTABLE}")
                set(PYTHON_FOUND TRUE)
                message(STATUS "Python standalone build downloaded to: ${python_standalone_SOURCE_DIR}")
                message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")
            else()
                message(WARNING "Failed to setup Python standalone build")
            endif()
        else()
            message(WARNING "Unsupported Linux architecture for automatic Python fetch: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        
    elseif(APPLE)
        # For macOS, download standalone Python build
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            set(PYTHON_URL "https://github.com/indygreg/python-build-standalone/releases/download/20231002/cpython-3.12.0+20231002-aarch64-apple-darwin-install_only.tar.gz")
        else()
            set(PYTHON_URL "https://github.com/indygreg/python-build-standalone/releases/download/20231002/cpython-3.12.0+20231002-x86_64-apple-darwin-install_only.tar.gz")
        endif()
        
        message(STATUS "Fetching Python ${PYTHON_VERSION} standalone build for macOS...")
        
        FetchContent_Declare(
            python_standalone
            URL ${PYTHON_URL}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        
        FetchContent_MakeAvailable(python_standalone)
        
        set(PYTHON_EXECUTABLE "${python_standalone_SOURCE_DIR}/bin/python3")
        
        if(EXISTS "${PYTHON_EXECUTABLE}")
            set(PYTHON_FOUND TRUE)
            message(STATUS "Python standalone build downloaded to: ${python_standalone_SOURCE_DIR}")
            message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")
        else()
            message(WARNING "Failed to setup Python standalone build")
        endif()
    else()
        message(WARNING "Unsupported platform for automatic Python fetch")
    endif()
    
    if(NOT PYTHON_FOUND)
        message(WARNING "Python could not be found or fetched. Please install Python manually.")
        message(WARNING "Visit https://www.python.org/downloads/ to download Python.")
    endif()
endif()

# Export variables for parent scope
set(PYTHON_FOUND ${PYTHON_FOUND} PARENT_SCOPE)
set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE} PARENT_SCOPE)
set(PYTHON_VERSION ${PYTHON_VERSION} PARENT_SCOPE)
