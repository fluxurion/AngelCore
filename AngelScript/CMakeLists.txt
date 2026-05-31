# AngelScript Integration CMakeLists.txt
# This file builds the AngelScript module and copies scripts to output

cmake_minimum_required(VERSION 3.24)

# AngelScript SDK Library
# SDK sources are in subdirectories (add_on/, angelscript/, etc.)
file(GLOB_RECURSE angelscript_sdk_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/SDK/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SDK/*.h
)

if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  enable_language(ASM_MASM)
  file(GLOB_RECURSE angelscript_sdk_asm_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/*_msvc_asm.asm
  )
  list(APPEND angelscript_sdk_sources ${angelscript_sdk_asm_sources})
endif()

# Exclude any CMake-generated files
list(FILTER angelscript_sdk_sources EXCLUDE REGEX ".*CMakeFiles.*")

add_library(angelscript_sdk STATIC ${angelscript_sdk_sources})

target_include_directories(angelscript_sdk
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/angelscript/include
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/add_on
)

# Disable warnings for SDK code (external library)
if(MSVC)
  target_compile_options(angelscript_sdk PRIVATE /W0)
else()
  target_compile_options(angelscript_sdk PRIVATE -w)
endif()

set_target_properties(angelscript_sdk PROPERTIES
  FOLDER "AngelScript"
)

# AngelScript Core Library (hooks, APIs, manager)
# Collect all source files except SDK
file(GLOB_RECURSE angelscript_core_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Remove SDK files from core (they're in the SDK library already)
list(FILTER angelscript_core_sources EXCLUDE REGEX ".*/SDK/.*")

# Remove the CMake-generated files if any
list(FILTER angelscript_core_sources EXCLUDE REGEX ".*CMakeFiles.*")

add_library(angelscript_core STATIC
  ${angelscript_core_sources}
)

target_include_directories(angelscript_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK
    ${CMAKE_CURRENT_SOURCE_DIR}/Hooks
    ${CMAKE_CURRENT_SOURCE_DIR}/API
    ${CMAKE_CURRENT_SOURCE_DIR}/Dispatch
)

target_link_libraries(angelscript_core
  PUBLIC
    angelscript_sdk
    game  # TrinityCore game library
    trinity-core-interface
    mysql # MySQL C API (for AngelDB raw connection)
)

target_compile_definitions(angelscript_core
  PUBLIC
    ANGELSCRIPT_INTEGRATION
)

set_target_properties(angelscript_core PROPERTIES
  FOLDER "AngelScript"
)

# Copy angelscripts folder to build output after build
# This ensures scripts are next to worldserver.exe
add_custom_command(TARGET angelscript_core POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/angelscripts
    $<TARGET_FILE_DIR:worldserver>/angelscripts
  COMMENT "Copying AngelScript files to worldserver output directory"
)

# Installation - install scripts alongside binaries
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/angelscripts/
  DESTINATION bin/angelscripts
  FILES_MATCHING
    PATTERN "*.as"
    PATTERN "*.txt"
    PATTERN "*.md"
    PATTERN "*.sql"
)

# Note: GroupSources is a TrinityCore macro - disabled for standalone
# GroupSources(${CMAKE_CURRENT_SOURCE_DIR}/angelscripts angelscripts)

# Count sources without listing them all
list(LENGTH angelscript_sdk_sources SDK_COUNT)
list(LENGTH angelscript_core_sources CORE_COUNT)

message(STATUS "AngelScript configured:")
message(STATUS "  SDK sources: ${SDK_COUNT} files")
message(STATUS "  Core sources: ${CORE_COUNT} files")
message(STATUS "  Scripts path: ${CMAKE_CURRENT_SOURCE_DIR}/angelscripts")
