add_library(sauro_station_utils STATIC
  JsonUtils.cpp
)
target_include_directories(sauro_station_utils PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(sauro_station_utils PUBLIC cxx_std_17)
