# Bootstrap-only library (shared by app + tests).
add_library(sauro_station_bootstrap STATIC
  BootstrapConfig.cpp
)
target_include_directories(sauro_station_bootstrap PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(sauro_station_bootstrap PUBLIC cxx_std_17)

# Minimal app entry point.
add_executable(sauro_station
  main.cpp
)
target_link_libraries(sauro_station PRIVATE sauro_station_bootstrap)
target_compile_features(sauro_station PRIVATE cxx_std_17)
