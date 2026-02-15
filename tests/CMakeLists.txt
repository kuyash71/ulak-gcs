add_executable(sauro_station_tests
  test_bootstrap_config.cpp
)
target_link_libraries(sauro_station_tests PRIVATE sauro_station_bootstrap)
target_compile_features(sauro_station_tests PRIVATE cxx_std_17)

add_test(
  NAME bootstrap_config_validation
  COMMAND $<TARGET_FILE:sauro_station_tests>
)
set_tests_properties(bootstrap_config_validation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(
  NAME app_startup_smoke
  COMMAND $<TARGET_FILE:sauro_station> --validate-only --config ${PROJECT_SOURCE_DIR}/config/settings.json
)

add_test(
  NAME app_rejects_missing_config
  COMMAND $<TARGET_FILE:sauro_station> --validate-only --config ${PROJECT_SOURCE_DIR}/config/does_not_exist.json
)
set_tests_properties(app_rejects_missing_config PROPERTIES
  WILL_FAIL TRUE
)
