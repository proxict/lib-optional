if (NOT TARGET gmock AND NOT TARGET gtest)
    add_subdirectory(external/googletest)
endif()

add_executable(unittests
    main.cpp
)

set_property(TARGET unittests PROPERTY CXX_STANDARD 11)
set_property(TARGET unittests PROPERTY CXX_STANDARD_REQUIRED TRUE)
set_property(TARGET unittests PROPERTY CXX_EXTENSIONS OFF)

target_compile_options(unittests
    PRIVATE -Wall -Wextra -Wpedantic --coverage
)
target_link_options(unittests
    PRIVATE --coverage
)

target_link_libraries(unittests
    PRIVATE lib-optional
    PRIVATE gmock
    PRIVATE gtest
)

add_custom_target(coverage
    COMMAND ${CMAKE_SOURCE_DIR}/test/coverage.sh ${CMAKE_SOURCE_DIR}/test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_dependencies(coverage unittests)
