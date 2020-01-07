This piece of hardware consists of a ICM-20948 9-axis inertial measurement unit (IMU), a Si705x digital temperature sensor, a nRF52840 system on chip (SoC) and battery management.

# Build
```bash
west build -b wort_swarm_node_hardware -- -DBOARD_ROOT=./
```

# Flash
```bash
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "init" -c "halt" -c "program build/zephyr/zephyr.hex verify" -c "reset run" -c "shutdown"
```
