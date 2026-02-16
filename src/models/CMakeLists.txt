add_library(sauro_station_models STATIC
  GcsCommand.cpp
  StreamMode.cpp
)
target_include_directories(sauro_station_models PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(sauro_station_models PUBLIC sauro_station_utils)
target_compile_features(sauro_station_models PUBLIC cxx_std_17)
