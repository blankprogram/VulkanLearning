
# example: a Catch2 unit test
find_package(Catch2 REQUIRED)
add_executable(tests
  test_main.cpp
  # …other test files…
)
target_link_libraries(tests PRIVATE
  engine
  Catch2::Catch2
)

include(CTest)
include(Catch)
catch_discover_tests(tests)
