#
# Makefile for the industrial I/O core.
#

obj-$(CONFIG_IIO) += industrialio.o
industrialio-y := industrialio-core.o industrialio-event.o inkern.o
industrialio-$(CONFIG_IIO_BUFFER) += industrialio-buffer.o
industrialio-$(CONFIG_IIO_TRIGGER) += industrialio-trigger.o

obj-$(CONFIG_IIO_CONFIGFS) += industrialio-configfs.o
obj-$(CONFIG_IIO_SW_DEVICE) += industrialio-sw-device.o
obj-$(CONFIG_IIO_SW_TRIGGER) += industrialio-sw-trigger.o
obj-$(CONFIG_IIO_TRIGGERED_EVENT) += industrialio-triggered-event.o

obj-y += accel/
obj-y += adc/
obj-y += amplifiers/
obj-y += buffer/
obj-y += chemical/
obj-y += dac/
obj-y += dummy/
obj-y += gyro/
obj-y += frequency/
obj-y += health/
obj-y += humidity/
obj-y += light/
obj-y += magnetometer/
obj-y += orientation/
obj-y += potentiometer/
obj-y += pressure/
obj-y += proximity/
obj-y += temperature/
obj-y += trigger/
