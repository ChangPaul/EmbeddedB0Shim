# EmbeddedB0Shim
Embedded system for B0 shimming for magnetic resonance imaging (MRI) applications.

Developed on Xilinx Zynq 7020 eval board. The board includes two CPUs: cpu0 and cpu1. They communicate 
through the on-chip memory (OCM) registers. CPU0 runs peta-linux and is available to the user, CPU1 controls
hardware peripherals with low-level script (to avoid OS overheads).

The project is organised as follows:
+ /cpu1_app
  Standalone C++ script for cpu1.
  + cpu0_spi_test.c
    Test script for the cpu1 that initializes the SPI.
  + lscript.ld
    Linker file.
  + main.c
    Main file that runs on cpu1. Initialises hardware peripherals. Initialises the registers of the
    digital-to-analog controller (DAC5360).
+ /DeviceTree
  Source files for the device tree blob.
  + pl.dtsi
    Programmable logic devices.
  + skeleton.dtsi
    Default skeleton (bare minimum needed to boot).
  + system.dtsi
    System specific device configurations.
  + zynq-7000.dtsi
    Default devices for Zynq 7000 eval boards.
+ /Hardware
  Hardware files for the programmable logic and processing system.
  + B0Ctrl_constraints.xdc
    System hardware constraints (e.g. physical constraints, timing constraints, etc).
  + B0Ctrl_design.tcl
    Script to generate the block design for the system.
  + PulseCounter_*.v
    Verilog files for a pulse counter implemented in the programmable logic. Only counts pulses that
    are on for longer than a certain time (to prevent false triggering).
  + SiemensGradient_*.v
    Verilog files to communication with Siemens gradient system.
+ /LinuxApps
  Linux C scripts for B0 shimming application.
  + /FidToCoeffs
    Calculated the spherical harmonic coefficients from FID signals.
  + /lsuio
    Script to access the UIOs.
  + /SysId
    Runs the program to acquire data for system identification.
+ /LinuxConfig
  Linux kernel config file
+ /uram_scripts
  Custom shell scripts copied onto the ramfs.
+ u-boot
  u-boot file for booting the system.
