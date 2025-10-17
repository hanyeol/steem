set(EQUIHASH_HEADERS_DIR)
set(EQUIHASH_SOURCES src/pow.cpp )

# Use reference implementation for ARM64 and other non-x86 architectures
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
  message(STATUS "Using equihash reference implementation for ARM64")
  set(EQUIHASH_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ref/include")
  list(APPEND EQUIHASH_SOURCES ref/src/blake2b-ref.c )
else()
  message(STATUS "Using equihash SSE implementation for x86/x64")
  set(EQUIHASH_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sse/include")
  list(APPEND EQUIHASH_SOURCES sse/src/blake2b.c )
endif()

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${EQUIHASH_HEADERS_DIR}/equihash/*.h" )

SET( sources ${EQUIHASH_SOURCES} ${HEADERS} )

add_library( equihash ${sources})

target_include_directories( equihash PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${EQUIHASH_HEADERS_DIR}" )

install( TARGETS
   equihash

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
