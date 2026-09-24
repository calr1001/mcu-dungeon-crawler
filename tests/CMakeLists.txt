# tests/CMakeLists.txt
add_executable(unit_tests test_combat.cpp)
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME GameEngineTests COMMAND unit_tests)