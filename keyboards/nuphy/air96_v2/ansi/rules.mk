SRC += user_kb.c
SRC += rf.c
# RGB matrix: QMK's IS31FL3733 driver behind a thin custom driver (is31fl3733_side_driver.c)
I2C_DRIVER_REQUIRED = yes
COMMON_VPATH += $(DRIVER_PATH)/led/issi
SRC += $(DRIVER_PATH)/led/issi/is31fl3733.c is31fl3733_side_driver.c
SRC += rgb.c

SRC += mcu_pwr.c sleep.c rf_driver.c rf_queue.c debounce.c
UART_DRIVER_REQUIRED = yes
EEPROM_DRIVER = wear_leveling
WEAR_LEVELING_DRIVER = embedded_flash
OS_DETECTION_ENABLE = yes

GCC_EXTRA_OPTIONS =
OPT = s $(GCC_EXTRA_OPTIONS)

CUSTOM_MATRIX = lite
SRC += matrix.c

