
################################################################
# This is a generated script based on design: B0Ctrl_design
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2015.2
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   puts "ERROR: This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source B0Ctrl_design_script.tcl

# If you do not already have a project created,
# you can create a project using the following command:
#    create_project project_1 myproj -part xc7z020clg484-1
#    set_property BOARD_PART xilinx.com:zc702:part0:1.2 [current_project]

# CHECKING IF PROJECT EXISTS
if { [get_projects -quiet] eq "" } {
   puts "ERROR: Please open or create a project!"
   return 1
}



# CHANGE DESIGN NAME HERE
set design_name B0Ctrl_design

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "ERROR: Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      puts "INFO: Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   puts "INFO: Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "ERROR: Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "ERROR: Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   puts "INFO: Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   puts "INFO: Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

puts "INFO: Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   puts $errMsg
   return $nRet
}

##################################################################
# DESIGN PROCs
##################################################################



# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set DDR [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddrx_rtl:1.0 DDR ]
  set FIXED_IO [ create_bd_intf_port -mode Master -vlnv xilinx.com:display_processing_system7:fixedio_rtl:1.0 FIXED_IO ]
  set GPIO [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 GPIO ]
  set SPI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:spi_rtl:1.0 SPI ]
  set UART [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:uart_rtl:1.0 UART ]

  # Create ports
  set RFTrigger [ create_bd_port -dir O RFTrigger ]
  set gnd_signal [ create_bd_port -dir O -from 3 -to 0 gnd_signal ]
  set grad_clk_i [ create_bd_port -dir I grad_clk_i ]
  set grad_clk_o [ create_bd_port -dir O grad_clk_o ]
  set grad_data_i [ create_bd_port -dir IO -from 19 -to 0 grad_data_i ]
  set grad_data_o [ create_bd_port -dir IO -from 19 -to 0 grad_data_o ]
  set grad_sel_i [ create_bd_port -dir IO -from 2 -to 0 grad_sel_i ]
  set grad_sel_o [ create_bd_port -dir IO -from 2 -to 0 grad_sel_o ]
  set grad_test_i [ create_bd_port -dir I -from 1 -to 0 grad_test_i ]
  set grad_test_o [ create_bd_port -dir O -from 1 -to 0 grad_test_o ]
  set inTrigger [ create_bd_port -dir I inTrigger ]

  # Create instance: PulseCounter, and set properties
  set PulseCounter [ create_bd_cell -type ip -vlnv mpi.localnet:user:PulseCounter:1.0 PulseCounter ]

  # Create instance: SiemensGradient, and set properties
  set SiemensGradient [ create_bd_cell -type ip -vlnv mpi.localnet:user:SiemensGradient:1.0 SiemensGradient ]

  # Create instance: ZynqPS, and set properties
  set ZynqPS [ create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 ZynqPS ]
  set_property -dict [ list CONFIG.PCW_CORE0_IRQ_INTR {0} CONFIG.PCW_CORE1_IRQ_INTR {1} CONFIG.PCW_EN_CLK1_PORT {0} CONFIG.PCW_FPGA1_PERIPHERAL_FREQMHZ {15} CONFIG.PCW_GPIO_EMIO_GPIO_ENABLE {1} CONFIG.PCW_GPIO_EMIO_GPIO_IO {3} CONFIG.PCW_IRQ_F2P_INTR {0} CONFIG.PCW_QSPI_GRP_SINGLE_SS_ENABLE {1} CONFIG.PCW_SPI1_PERIPHERAL_ENABLE {1} CONFIG.PCW_UART0_PERIPHERAL_ENABLE {1} CONFIG.PCW_USE_FABRIC_INTERRUPT {0} CONFIG.preset {ZC702}  ] $ZynqPS

  # Create instance: ZynqPS_axi_periph, and set properties
  set ZynqPS_axi_periph [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 ZynqPS_axi_periph ]
  set_property -dict [ list CONFIG.NUM_MI {2}  ] $ZynqPS_axi_periph

  # Create instance: rst_ZynqPS_50M, and set properties
  set rst_ZynqPS_50M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_ZynqPS_50M ]

  # Create interface connections
  connect_bd_intf_net -intf_net ZynqPS_M_AXI_GP0 [get_bd_intf_pins ZynqPS/M_AXI_GP0] [get_bd_intf_pins ZynqPS_axi_periph/S00_AXI]
  connect_bd_intf_net -intf_net ZynqPS_SPI_1 [get_bd_intf_ports SPI] [get_bd_intf_pins ZynqPS/SPI_1]
  connect_bd_intf_net -intf_net ZynqPS_axi_periph_M00_AXI [get_bd_intf_pins SiemensGradient/S00_AXI] [get_bd_intf_pins ZynqPS_axi_periph/M00_AXI]
  connect_bd_intf_net -intf_net ZynqPS_axi_periph_M01_AXI [get_bd_intf_pins PulseCounter/S00_AXI] [get_bd_intf_pins ZynqPS_axi_periph/M01_AXI]
  connect_bd_intf_net -intf_net processing_system7_0_DDR [get_bd_intf_ports DDR] [get_bd_intf_pins ZynqPS/DDR]
  connect_bd_intf_net -intf_net processing_system7_0_FIXED_IO [get_bd_intf_ports FIXED_IO] [get_bd_intf_pins ZynqPS/FIXED_IO]
  connect_bd_intf_net -intf_net processing_system7_0_GPIO_0 [get_bd_intf_ports GPIO] [get_bd_intf_pins ZynqPS/GPIO_0]
  connect_bd_intf_net -intf_net processing_system7_0_UART_0 [get_bd_intf_ports UART] [get_bd_intf_pins ZynqPS/UART_0]

  # Create port connections
  connect_bd_net -net Net [get_bd_ports grad_data_i] [get_bd_pins SiemensGradient/grad_data_i]
  connect_bd_net -net Net1 [get_bd_ports grad_data_o] [get_bd_pins SiemensGradient/grad_data_o]
  connect_bd_net -net Net2 [get_bd_ports grad_sel_i] [get_bd_pins SiemensGradient/grad_sel_i]
  connect_bd_net -net Net3 [get_bd_ports grad_sel_o] [get_bd_pins SiemensGradient/grad_sel_o]
  connect_bd_net -net SiemensGradient_0_gnd_signal [get_bd_ports gnd_signal] [get_bd_pins SiemensGradient/gnd_signal]
  connect_bd_net -net SiemensGradient_0_grad_clk_o [get_bd_ports grad_clk_o] [get_bd_pins SiemensGradient/grad_clk_o]
  connect_bd_net -net SiemensGradient_0_grad_test_o [get_bd_ports grad_test_o] [get_bd_pins SiemensGradient/grad_test_o]
  connect_bd_net -net ZynqPS_FCLK_RESET0_N [get_bd_pins ZynqPS/FCLK_RESET0_N] [get_bd_pins rst_ZynqPS_50M/ext_reset_in]
  connect_bd_net -net grad_clk_i_1 [get_bd_ports grad_clk_i] [get_bd_pins SiemensGradient/grad_clk_i]
  connect_bd_net -net grad_test_i_1 [get_bd_ports grad_test_i] [get_bd_pins SiemensGradient/grad_test_i]
  connect_bd_net -net inTrigger_1 [get_bd_ports inTrigger] [get_bd_pins PulseCounter/inTrigger]
  connect_bd_net -net processing_system7_0_FCLK_CLK0 [get_bd_pins PulseCounter/s00_axi_aclk] [get_bd_pins SiemensGradient/s00_axi_aclk] [get_bd_pins ZynqPS/FCLK_CLK0] [get_bd_pins ZynqPS/M_AXI_GP0_ACLK] [get_bd_pins ZynqPS_axi_periph/ACLK] [get_bd_pins ZynqPS_axi_periph/M00_ACLK] [get_bd_pins ZynqPS_axi_periph/M01_ACLK] [get_bd_pins ZynqPS_axi_periph/S00_ACLK] [get_bd_pins rst_ZynqPS_50M/slowest_sync_clk]
  connect_bd_net -net processing_system7_0_TTC0_WAVE2_OUT [get_bd_ports RFTrigger] [get_bd_pins ZynqPS/TTC0_WAVE2_OUT]
  connect_bd_net -net rst_ZynqPS_50M_interconnect_aresetn [get_bd_pins ZynqPS_axi_periph/ARESETN] [get_bd_pins rst_ZynqPS_50M/interconnect_aresetn]
  connect_bd_net -net rst_ZynqPS_50M_peripheral_aresetn [get_bd_pins PulseCounter/s00_axi_aresetn] [get_bd_pins SiemensGradient/s00_axi_aresetn] [get_bd_pins ZynqPS_axi_periph/M00_ARESETN] [get_bd_pins ZynqPS_axi_periph/M01_ARESETN] [get_bd_pins ZynqPS_axi_periph/S00_ARESETN] [get_bd_pins rst_ZynqPS_50M/peripheral_aresetn]

  # Create address segments
  create_bd_addr_seg -range 0x10000 -offset 0x43C10000 [get_bd_addr_spaces ZynqPS/Data] [get_bd_addr_segs PulseCounter/S00_AXI/S00_AXI_reg] SEG_PulseCounter_0_S00_AXI_reg
  create_bd_addr_seg -range 0x10000 -offset 0x43C00000 [get_bd_addr_spaces ZynqPS/Data] [get_bd_addr_segs SiemensGradient/S00_AXI/S00_AXI_reg] SEG_SiemensGradient_0_S00_AXI_reg
  

  # Restore current instance
  current_bd_instance $oldCurInst

  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


