cmake_minimum_required(VERSION 3.8.2)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD wort_swarm_node_hardware)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(peripheral)

add_subdirectory_ifdef(CONFIG_ICM20948 drivers/sensor/icm20948)
add_subdirectory_ifdef(CONFIG_SI705X drivers/sensor/si705x)

target_sources(app PRIVATE
  src/main.c
  
  src/ass.c
  src/bas.c
  src/cts.c
  
  src/node.c
  
  src/battery.c
)

#zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
