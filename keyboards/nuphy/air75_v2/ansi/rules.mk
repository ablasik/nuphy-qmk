SRC += user_kb.c
SRC += rf.c
# RGB matrix: QMK's WS2812 driver behind a thin custom driver (ws2812_side_driver.c)
WS2812_DRIVER_REQUIRED = yes
SRC += ws2812_side_driver.c
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


SRC += keyboards/nuphy/common/connection_actions.c
