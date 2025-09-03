target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        OpenIDAuthManager.h
        OpenIDAuthManager.cpp
        UavModelDecryptionManager.h
        UavModelDectyptionManager.cpp
        YubiKeyECDF.h
        YubiKeyECDF.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX AND NOT APPLE)
    # Linux - use pkg-config to find libpcsclite
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PCSCLITE REQUIRED libpcsclite)

    set(PCSC_LIBRARIES ${PCSCLITE_LIBRARIES})
    set(PCSC_INCLUDE_DIRS ${PCSCLITE_INCLUDE_DIRS})
elseif(APPLE)
    # TODO: I am not sure that this code works for macos. Need to verify
    # macOS - PC/SC is part of the system framework
    find_library(PCSC_LIBRARIES PCSC)
    set(PCSC_INCLUDE_DIRS "")
elseif(WIN32)
    # TODO: I am not sure that this code works for Windows. Need to verify
    # Windows - winscard is part of the system
    set(PCSC_LIBRARIES winscard)
    set(PCSC_INCLUDE_DIRS "")
endif()

# Link libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${PCSC_LIBRARIES})

# Include directories
if(PCSC_INCLUDE_DIRS)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${PCSC_INCLUDE_DIRS})
endif()

# Compiler flags
if(UNIX)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${PCSCLITE_CFLAGS_OTHER})
endif()
