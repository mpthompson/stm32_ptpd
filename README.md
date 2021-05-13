# PTPD v2 projects for ST NUCLEO-F429ZI development board

This code improves upon my previous PTPD example libraries for STM32 devices in the following ways:

 - Provides an example of a PTPD master and slave
 - Projects for ST NUCLEO-F429ZI dev board which is more widely available
 - Ported PTPD v2 code has been improved and has better comments
 - Supports the STM32 F4 HAL library rather than the older standard library
 - Project files for Keil uVision and Linux friendly Makefile
 - Improved telnet and serial shell for testing
 - Easier network configuration in main.c

## PTPD Slave

/projects/nucleo_ptpd_slave

Using the slave example is fairly simple. Modify the network configuration at the
bottom of the main.c file, compile and flash to a NUCLEO-F429ZI development board.
Then plug the board into a network with a functioning PTPD master configured.
Power can then be applied and the system shell accessed via the virtual serial port
over the debug USB connection or via telnet to the board IP address.

Once in the shell, the 'dmesg' command should give you the following output
after a few minutes indicating the PTPD slave is configured and synchronized
with the PTPD master on the network.

    > dmesg
    <134>Jan  1 00:00:01 firmware: NETWORK: address is 192.168.1.75
    <134>Jan  1 00:00:01 firmware: NETWORK: netmask is 255.255.255.0
    <134>Jan  1 00:00:01 firmware: NETWORK: gateway is 192.168.1.1
    <134>Jan  1 00:00:01 firmware: NETWORK: link is up
    <133>Jan  1 00:00:01 firmware: PTPD: entering INITIALIZING state
    <133>Jan  1 00:00:01 firmware: PTPD: entering LISTENING state
    <134>Jan  1 00:00:01 firmware: TELNET: waiting for connection
    <133>Jan  1 00:00:02 firmware: PTPD: entering UNCALIBRATED state
    <133>May 13 05:18:05 firmware: PTPD: setting Thu May 13 05:18:05 UTC 2021
    <133>May 13 05:18:20 firmware: PTPD: entering SLAVE state
    >

The 'ptpd' command will give the current status of the PTPD synchronization.

    > ptpd
    master id: 000db9fffe0cde34
    state: slave
    mode: end to end
    path delay: 104969 nsec
    offset: -4866 nsec
    drift: -3.620 ppm
    >

## PTPD Master

/projects/nucleo_ptpd_master

Note the PTPD master example synchronizes off the GPS pulse-per-second signal from
a Venus638FLPx GPS Receiver which was readily available in the past, but may be
harder to find these days. I would welcome enhancements that support a wider
variety of GPS devices.

With the master project, the precision timer on the STM32F4 MCU is intended to
be synchronized with the GPS pulse-per-second signal and any slaves on the network
are then synchronized against the precision timer.

The master code is not very well tested and your mileage may vary.
