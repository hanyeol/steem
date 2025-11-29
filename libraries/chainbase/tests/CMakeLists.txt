add_executable( chainbase_tests chainbase_tests.cpp database_test.cpp )
target_link_libraries( chainbase_tests chainbase ${PLATFORM_SPECIFIC_LIBS} )

