# Makefile for PTPD slave for NUCLEO-F429ZI.

# Project name
NAME = nucleo_ptpd_slave

# Outputs
BINMAP = $(NAME).map
BINELF = $(NAME).elf
BINHEX = $(NAME).hex
BINBIN = $(NAME).bin
BINCRC = $(NAME).crc
BINMAP = $(NAME).map

# GNU ARM Embedded Toolchain
PREFIX = arm-none-eabi-
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
CXX = $(GCC_PATH)/$(PREFIX)g++
LD = $(GCC_PATH)/$(PREFIX)ld
AR = $(GCC_PATH)/$(PREFIX)ar
AS = $(GCC_PATH)/$(PREFIX)as
NM = $(GCC_PATH)/$(PREFIX)nm
SIZE = $(GCC_PATH)/$(PREFIX)size
A2L = $(GCC_PATH)/$(PREFIX)addr2line
OBJCOPY = $(GCC_PATH)/$(PREFIX)objcopy
OBJDUMP = $(GCC_PATH)/$(PREFIX)objdump
else
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LD = $(PREFIX)ld
AR = $(PREFIX)ar
AS = $(PREFIX)as
NM = $(PREFIX)nm
SIZE = $(PREFIX)size
A2L = $(PREFIX)addr2line
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
endif

# Startup
SRCS = ../shared_stm32/startup/gcc/startup_stm32f429xx.s

# Application
SRCS += ./src/main.c
SRCS += ./src/hal_system.c

# Application Shared
SRCS += ../shared/event.c
SRCS += ../shared/outputf.c
SRCS += ../shared/peek.c
SRCS += ../shared/reboot.c
SRCS += ../shared/shell.c
SRCS += ../shared/syslog.c
SRCS += ../shared/telnet.c
SRCS += ../shared/uptime.c

# Application Shared Nucleo
SRCS += ../shared_nucleo/buttons.c
SRCS += ../shared_nucleo/leds.c

# Application Shared STM32
SRCS += ../shared_stm32/blink.c
SRCS += ../shared_stm32/clocks.c
SRCS += ../shared_stm32/console.c
SRCS += ../shared_stm32/delay.c
SRCS += ../shared_stm32/ethptp.c
SRCS += ../shared_stm32/extint.c
SRCS += ../shared_stm32/hardtime.c
SRCS += ../shared_stm32/network.c
SRCS += ../shared_stm32/ntime.c
SRCS += ../shared_stm32/random.c
SRCS += ../shared_stm32/systime.c
SRCS += ../shared_stm32/system_stm32f4xx.c
SRCS += ../shared_stm32/tick.c
SRCS += ../shared_stm32/watchdog.c

# Utilities
SRCS += ../shared/newlib_stubs.c

# Drivers
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_adc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_crc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dac.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dma.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dma2d.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_exti.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fmc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fsmc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_gpio.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_i2c.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_lptim.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_pwr.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rcc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rng.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rtc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_spi.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_tim.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usart.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c
SRCS += ../../libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_utils.c

# NT Shell
SRCS += ../../libraries/ntshell-v0.3.1/lib/core/ntlibc.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/util/ntopt.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/core/ntshell.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/util/ntstdio.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/core/text_editor.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/core/text_history.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/core/vtrecv.c
SRCS += ../../libraries/ntshell-v0.3.1/lib/core/vtsend.c

# PTPD
SRCS += ../shared/ptpd/src/ptpd_arith.c
SRCS += ../shared/ptpd/src/ptpd_bmc.c
SRCS += ../shared/ptpd/src/ptpd_main.c
SRCS += ../shared/ptpd/src/ptpd_msg.c
SRCS += ../shared/ptpd/src/ptpd_net.c
SRCS += ../shared/ptpd/src/ptpd_protocol.c
SRCS += ../shared/ptpd/src/ptpd_servo.c
SRCS += ../shared/ptpd/src/ptpd_time.c
SRCS += ../shared/ptpd/src/ptpd_timer.c

# LWIP API
SRCS += ../../libraries/LWIP-2.1.2/src/api/api_lib.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/api_msg.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/err.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/netbuf.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/netdb.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/netifapi.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/sockets.c
SRCS += ../../libraries/LWIP-2.1.2/src/api/tcpip.c

# LWIP Core
SRCS += ../../libraries/LWIP-2.1.2/src/core/altcp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/altcp_alloc.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/altcp_tcp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/def.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/dns.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/inet_chksum.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/init.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ip.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/mem.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/memp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/netif.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/pbuf.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/raw.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/stats.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/sys.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/tcp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/tcp_in.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/tcp_out.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/timeouts.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/udp.c

# LWIP Core IPv4
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/autoip.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/dhcp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/etharp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/icmp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/igmp.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/ip4.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/ip4_addr.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv4/ip4_frag.c

# LWIP Core IPv6
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/dhcp6.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/ethip6.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/icmp6.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/inet6.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/ip6.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/ip6_addr.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/ip6_frag.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/mld6.c
SRCS += ../../libraries/LWIP-2.1.2/src/core/ipv6/nd6.c

# LWIP Netif
SRCS += ../../libraries/LWIP-2.1.2/src/netif/ethernet.c
SRCS += ../../libraries/LWIP-2.1.2/src/netif/lowpan6.c

# LWIP Port
SRCS += ../shared_stm32/lwip_port/ethernetif.c
SRCS += ../shared_stm32/lwip_port/sys_arch.c

# LWIP Apps
SRCS += ../../libraries/LWIP-2.1.2/src/apps/mdns/mdns.c

# RTOS Config
SRCS += ./src/RTX_Config.c

# RTOS Kernel
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_delay.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_evflags.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_evr.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_kernel.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_lib.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_memory.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_mempool.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_msgqueue.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_mutex.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_semaphore.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_system.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_thread.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/rtx_timer.c
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/Source/os_systick.c

# RTOS IRQ
SRCS += ../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source/GCC/irq_cm4f.S

# List of directories that contain source code
SRC_PATHS = $(sort $(dir $(SRCS)))

# Specify the output path
OUTPATH = build

# Create the object list from the sources
OBJS = $(subst .s,.o,$(subst .S,.o,$(subst .c,.o,$(subst .cpp,.o,$(addprefix $(OUTPATH)/, $(notdir $(SRCS)))))))

#$(info SRCS=$(SRCS))
#$(info OBJS=$(OBJS))
#$(info SRC_PATHS=$(SRC_PATHS))

# Where to find source files.
vpath %.cpp $(SRC_PATHS)
vpath %.c $(SRC_PATHS)
vpath %.s $(SRC_PATHS)
vpath %.S $(SRC_PATHS)

# Includes
INCLUDES = -I.
INCLUDES += -I./src
INCLUDES += -I../shared
INCLUDES += -I../shared/ptpd/src
INCLUDES += -I../shared_stm32
INCLUDES += -I../shared_stm32/lwip_port
INCLUDES += -I../shared_nucleo
INCLUDES += -I../../libraries/CMSIS/5.4.0/CMSIS/Core/Include
INCLUDES += -I../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/Include
INCLUDES += -I../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Include
INCLUDES += -I../../libraries/CMSIS/5.4.0/CMSIS/RTOS2/RTX/Source
INCLUDES += -I../../libraries/CMSIS/5.4.0_ST/Device/ST/STM32F4xx/Include
INCLUDES += -I../../libraries/STM32F4xx_HAL_Driver/Inc
INCLUDES += -I../../libraries/LWIP-2.1.2/src
INCLUDES += -I../../libraries/LWIP-2.1.2/src/include
INCLUDES += -I../../libraries/ntshell-v0.3.1/lib/core
INCLUDES += -I../../libraries/ntshell-v0.3.1/lib/util

OPTFLAGS = -Os

MCFLAGS = -mcpu=cortex-m4
MCFLAGS += -mthumb
MCFLAGS += -mlittle-endian
MCFLAGS += -mfpu=fpv4-sp-d16
MCFLAGS += -mfloat-abi=hard
MCFLAGS += -mapcs-frame
MCFLAGS += -MD
MCFLAGS += -Wall

FFLAGS = -fno-common
FFLAGS += -ffunction-sections
FFLAGS += -fdata-sections
FFLAGS += -ffreestanding
FFLAGS += -fno-builtin

WFLAGS = -Wno-strict-aliasing

DEFINES = -DSTM32F429xx
DEFINES += -DNDEBUG
DEFINES += -DUSE_STM32F4XX_NUCLEO_144
DEFINES += -DUSE_HAL_DRIVER
DEFINES += -DUSE_FULL_LL_DRIVER
DEFINES += -D__CMSIS_RTOS
DEFINES += -DVECT_TAB_OFFSET=0x0
DEFINES += -DEVR_RTX_DISABLE

LIBS = -lm

# CFLAGS
CFLAGS = -c $(OPTFLAGS) $(MCFLAGS) $(FFLAGS) $(WFLAGS) $(DEFINES) $(INCLUDES)
CXXFLAGS = -c $(OPTFLAGS) $(MCFLAGS) $(FFLAGS) $(WFLAGS) $(DEFINES) $(INCLUDES) -std=c++11

# LINKER FLAGS
LDSCRIPT = nucleo_stm32f429.ld
LDFLAGS = -T $(LDSCRIPT)
LDFLAGS += $(MCFLAGS)
LDFLAGS += $(FFLAGS)
LDFLAGS += $(INCLUDES_LIBS)
LDFLAGS += $(LIBS)
LDFLAGS += -specs=nano.specs
#LDFLAGS += -Wl,--gc-sections -specs=nano.specs
LDFLAGS += -Wl,-Map=$(OUTPATH)/$(BINMAP),--cref 

###
# Build Rules
.PHONY: all release debug clean

all: $(OUTPATH) release

debug: CFLAGS += -DDEBUG -g -g3 -gdwarf-2 -gdwarf-3
debug: CXXFLAGS += -DDEBUG -g -g3 -gdwarf-2 -gdwarf-3
debug: LDFLAGS += -g -g3 -gdwarf-2 -gdwarf-3
debug: release

release: $(OUTPATH)/$(BINHEX) $(OUTPATH)/$(BINBIN) $(OUTPATH)/$(BINCRC)

$(OUTPATH):
	mkdir -p $(OUTPATH)

$(OUTPATH)/$(BINHEX): $(OUTPATH)/$(BINELF)
	$(OBJCOPY) -O ihex $< $@

$(OUTPATH)/$(BINBIN): $(OUTPATH)/$(BINELF)
	$(OBJCOPY) -O binary $< $@

$(OUTPATH)/$(BINCRC): $(OUTPATH)/$(BINBIN)
	cksum < $< > $@

##
# C linking is used. If C++ linker is required, change:
#   $(CC) $(OBJS) $(LDFLAGS) -o $@
# to:
#   $(CXX) $(OBJS) $(LDFLAGS) -o $@
#
$(OUTPATH)/$(BINELF): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $(OUTPATH)/$(BINELF)

$(OUTPATH)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OUTPATH)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OUTPATH)/%.o : %.s
	$(CC) $(CFLAGS) $< -o $@

$(OUTPATH)/%.o : %.S
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS)
	rm -f $(OUTPATH)/$(BINELF)
	rm -f $(OUTPATH)/$(BINMAP)
	rm -f $(OUTPATH)/$(BINHEX)
	rm -f $(OUTPATH)/$(BINBIN)
	rm -f $(OUTPATH)/$(BINCRC)
	rm -f $(OUTPATH)/$(BINMAP)
	rm -f $(OUTPATH)/*.d
	rm -f $(OUTPATH)/*.su

flash: $(OUTPATH)/$(BINHEX) 
	st-flash --reset --format ihex write $(OUTPATH)/$(BINHEX)


