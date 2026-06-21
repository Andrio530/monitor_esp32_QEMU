MONITOR_ESP32_VERSION = 1.0
MONITOR_ESP32_SITE = /home/andrio/embarcados/monitor_esp32/src
MONITOR_ESP32_SITE_METHOD = local

define MONITOR_ESP32_BUILD_CMDS
    $(TARGET_CC) $(TARGET_CFLAGS) -o $(@D)/monitor_esp32 \
        $(MONITOR_ESP32_SITE)/monitor_esp32.c
endef

define MONITOR_ESP32_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/monitor_esp32 \
        $(TARGET_DIR)/usr/bin/monitor_esp32
    $(INSTALL) -D -m 0755 $(MONITOR_ESP32_SITE)/../init/S99monitor \
        $(TARGET_DIR)/etc/init.d/S99monitor
endef

$(eval $(generic-package))
