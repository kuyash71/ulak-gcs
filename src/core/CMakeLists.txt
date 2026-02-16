add_library(sauro_station_core STATIC
  AppConfig.cpp
  ConfigLoader.cpp
  ProfileLoader.cpp
)
target_include_directories(sauro_station_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(sauro_station_core PUBLIC sauro_station_utils)
target_compile_features(sauro_station_core PUBLIC cxx_std_17)
