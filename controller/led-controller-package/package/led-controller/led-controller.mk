################################################################################
#
# led-controller
#
################################################################################

LED_CONTROLLER_VERSION = 1.0
LED_CONTROLLER_SITE = $(BR2_EXTERNAL_LED_CONTROLLER_PATH)/package/led-controller/src
LED_CONTROLLER_SITE_METHOD = local
LED_CONTROLLER_DEPENDENCIES = libgpiod

define LED_CONTROLLER_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(@D)/led-controller \
		$(@D)/controller_precise.c \
		$(@D)/gpio_real.c \
		-lgpiod -lrt
endef

define LED_CONTROLLER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/led-controller \
		$(TARGET_DIR)/usr/bin/led-controller
	$(INSTALL) -D -m 0755 \
		$(BR2_EXTERNAL_LED_CONTROLLER_PATH)/package/led-controller/S90led-controller \
		$(TARGET_DIR)/etc/init.d/S90led-controller
endef

$(eval $(generic-package))

