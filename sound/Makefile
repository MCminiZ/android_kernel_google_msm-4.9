# Makefile for the Linux sound card driver
#

obj-$(CONFIG_SOUND) += soundcore.o
obj-$(CONFIG_SOUND_PRIME) += oss/
obj-$(CONFIG_DMASOUND) += oss/
obj-$(CONFIG_SND) += core/ i2c/ drivers/ pci/ ppc/ arm/ sh/ synth/ usb/ \
	firewire/ sparc/ spi/ parisc/ mips/ soc/ atmel/ hda/
obj-$(CONFIG_SND_AOA) += aoa/

# This one must be compilable even if sound is configured out
obj-$(CONFIG_AC97_BUS) += ac97_bus.o

ifeq ($(CONFIG_SND),y)
  obj-y += last.o
endif

soundcore-objs  := sound_core.o
