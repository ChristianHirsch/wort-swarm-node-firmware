#zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ICM20948 icm20948.c)
zephyr_library_sources_ifdef(CONFIG_ICM20948 icm20948_i2c.c)
zephyr_library_sources_ifdef(CONFIG_ICM20948 icm20948_spi.c)
