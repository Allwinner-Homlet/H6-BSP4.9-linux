#
# Makefile for sunxi specific clk
#

obj-y += clk-sunxi.o clk-factors.o clk-periph.o
obj-${CONFIG_COMMON_CLK_DEBUG} += clk-debugfs.o
obj-${CONFIG_ARCH_SUN50IW3} += clk-sun50iw3.o
obj-${CONFIG_ARCH_SUN50IW1} += clk-sun50iw1.o
obj-${CONFIG_ARCH_SUN50IW6} += clk-sun50iw6.o
obj-${CONFIG_ARCH_SUN8IW6} += clk-sun8iw6.o
ifeq ($(CONFIG_SUNXI_ARISC), y)
obj-${CONFIG_ARCH_SUN8IW6} += clk-ac100.o
endif
>>>>>>> sun8iw6 & sun8iw15: support linux-4.9 platform
obj-${CONFIG_ARCH_SUN8IW7} += clk-sun8iw7.o
obj-${CONFIG_ARCH_SUN8IW11} += clk-sun8i11.o
obj-${CONFIG_ARCH_SUN8IW12} += clk-sun8iw12.o
obj-${CONFIG_ARCH_SUN8IW15} += clk-sun8iw15.o
