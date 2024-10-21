ARCHS = arm64
DEBUG = 0
FINALPACKAGE = 1
FOR_RELEASE = 1

TWEAK_NAME = RuntimeDumper

$(TWEAK_NAME)_CFLAGS = -fobjc-arc
$(TWEAK_NAME)_CCFLAGS = -std=c++0x -std=gnu++0x

include $(THEOS)/makefiles/common.mk

$(TWEAK_NAME)_FILES = Main.mm $(wildcard SCLAlertView/*.m)

$(TWEAK_NAME)_FRAMEWORKS = UIKit Foundation

include $(THEOS_MAKE_PATH)/tweak.mk
