# Single binary for bootstrap/config validation tests.
add_executable(sauro_station_tests
  test_bootstrap_config.cpp
)
target_link_libraries(sauro_station_tests PRIVATE sauro_station_bootstrap)

add_executable(sauro_station_config_tests
  test_config_loader.cpp
)
target_link_libraries(sauro_station_config_tests PRIVATE sauro_station_core)

add_executable(sauro_station_command_tests
  test_command_serialize.cpp
)
target_link_libraries(sauro_station_command_tests PRIVATE sauro_station_models)

add_executable(sauro_station_profile_policy_tests
  profile_data_parse.cpp
)
target_link_libraries(sauro_station_profile_policy_tests PRIVATE sauro_station_core)

add_executable(sauro_station_error_countdown_tests
  error_counter.cpp
)
target_link_libraries(sauro_station_error_countdown_tests PRIVATE sauro_station_core)

add_executable(sauro_station_panic_tests
  panic_button.cpp
)
target_link_libraries(sauro_station_panic_tests PRIVATE sauro_station_core)
target_compile_features(sauro_station_tests PRIVATE cxx_std_17)

# Config validation tests.
add_test(
  NAME bootstrap_config_validation
  COMMAND $<TARGET_FILE:sauro_station_tests>
)
set_tests_properties(bootstrap_config_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# Startup smoke test (validate-only mode).
add_test(
  NAME app_startup_smoke
  COMMAND $<TARGET_FILE:sauro_station> --validate-only --config ${PROJECT_SOURCE_DIR}/config/settings.json
)

# Negative case: missing config should fail with non-zero exit.
add_test(
  NAME app_rejects_missing_config
  COMMAND $<TARGET_FILE:sauro_station> --validate-only --config ${PROJECT_SOURCE_DIR}/config/does_not_exist.json
)
set_tests_properties(app_rejects_missing_config PROPERTIES
  WILL_FAIL TRUE
)

add_test(
  NAME config_loader_validation
  COMMAND $<TARGET_FILE:sauro_station_config_tests>
)
set_tests_properties(config_loader_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
  NAME command_serialization_validation
  COMMAND $<TARGET_FILE:sauro_station_command_tests>
)
set_tests_properties(command_serialization_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
  NAME profile_policy_validation
  COMMAND $<TARGET_FILE:sauro_station_profile_policy_tests>
)
set_tests_properties(profile_policy_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
  NAME error_countdown_validation
  COMMAND $<TARGET_FILE:sauro_station_error_countdown_tests>
)
set_tests_properties(error_countdown_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
  NAME panic_manager_validation
  COMMAND $<TARGET_FILE:sauro_station_panic_tests>
)
set_tests_properties(panic_manager_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
