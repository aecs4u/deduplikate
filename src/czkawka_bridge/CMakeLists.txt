cmake_minimum_required(VERSION 3.16)

# Build the Rust library
set(CZKAWKA_BRIDGE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CZKAWKA_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../czkawka/czkawka_core)

# Determine build type for Cargo
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_BUILD_TYPE "debug")
    set(RUST_TARGET_DIR "${CZKAWKA_BRIDGE_DIR}/target/debug")
else()
    set(CARGO_BUILD_TYPE "release")
    set(RUST_TARGET_DIR "${CZKAWKA_BRIDGE_DIR}/target/release")
    set(CARGO_RELEASE_FLAG "--release")
endif()

# Generate C header from Rust
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/czkawka_bridge.h
    COMMAND cbindgen --config cbindgen.toml --crate czkawka_bridge --output czkawka_bridge.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
    COMMENT "Generating C header from Rust"
)

# Build Rust library
add_custom_command(
    OUTPUT ${RUST_TARGET_DIR}/libczkawka_bridge.a
    COMMAND cargo build ${CARGO_RELEASE_FLAG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
        ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
    COMMENT "Building Rust bridge library"
)

# Create imported library target
add_custom_target(czkawka_bridge_rust
    DEPENDS
        ${RUST_TARGET_DIR}/libczkawka_bridge.a
        ${CMAKE_CURRENT_SOURCE_DIR}/czkawka_bridge.h
)

add_library(czkawka_bridge STATIC IMPORTED GLOBAL)
add_dependencies(czkawka_bridge czkawka_bridge_rust)

set_target_properties(czkawka_bridge PROPERTIES
    IMPORTED_LOCATION ${RUST_TARGET_DIR}/libczkawka_bridge.a
)

# Link system libraries required by Rust
if(UNIX AND NOT APPLE)
    target_link_libraries(czkawka_bridge INTERFACE pthread dl)
elseif(APPLE)
    target_link_libraries(czkawka_bridge INTERFACE pthread)
elseif(WIN32)
    target_link_libraries(czkawka_bridge INTERFACE ws2_32 userenv)
endif()
