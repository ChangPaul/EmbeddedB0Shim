
`timescale 1 ns / 1 ps

	module SiemensGradient_v1_0 #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line


		// Parameters of Axi Slave Bus Interface S00_AXI
		parameter integer C_S00_AXI_DATA_WIDTH	= 32,
		parameter integer C_S00_AXI_ADDR_WIDTH	= 4
	)
	(
		// Users to add ports here
		// Users to add ports here
        inout  wire [19 : 0] grad_data_i,
        inout  wire [19 : 0] grad_data_o,
        inout  wire [ 2 : 0] grad_sel_i,
        inout  wire [ 2 : 0] grad_sel_o,
        input  wire          grad_clk_i,
        output wire          grad_clk_o,
        input  wire [ 1 : 0] grad_test_i,
        output wire [ 1 : 0] grad_test_o,
        output wire [ 3 : 0] gnd_signal,
		// User ports ends
		// Do not modify the ports beyond this line


		// Ports of Axi Slave Bus Interface S00_AXI
		input wire  s00_axi_aclk,
		input wire  s00_axi_aresetn,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
		input wire [2 : 0] s00_axi_awprot,
		input wire  s00_axi_awvalid,
		output wire  s00_axi_awready,
		input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
		input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
		input wire  s00_axi_wvalid,
		output wire  s00_axi_wready,
		output wire [1 : 0] s00_axi_bresp,
		output wire  s00_axi_bvalid,
		input wire  s00_axi_bready,
		input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
		input wire [2 : 0] s00_axi_arprot,
		input wire  s00_axi_arvalid,
		output wire  s00_axi_arready,
		output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
		output wire [1 : 0] s00_axi_rresp,
		output wire  s00_axi_rvalid,
		input wire  s00_axi_rready
	);
	
	assign gnd_signal = 4'b0;
    wire [2:0] bgrad_sel_i;
    wire [2:0] bgrad_sel_o;
    wire [C_S00_AXI_DATA_WIDTH-1:0] bgrad_data_i;
    wire [19:0] bgrad_data_o;
    wire [1:0] bgrad_test_i;
    wire [1:0] bgrad_test_o;
    wire inv_test = !bgrad_test_i[0];
    
    assign bgrad_data_i[C_S00_AXI_DATA_WIDTH-1:20] = 12'b0;
    
    IBUF IBUF_clkin         (.I(grad_clk_i),        .O(bgrad_clk_i));
    OBUF OBUF_clkout        (.I(bgrad_clk_o),       .O(grad_clk_o));
    OBUF OBUF_testout[1:0]  (.I(bgrad_test_o),      .O(grad_test_o));
    IBUF IBUF_testin[1:0]   (.I(grad_test_i[1:0]),  .O(bgrad_test_i[1:0]));

    wire [2:0] cond_sel0;    wire [2:0] cond_sel1;
    wire [19:0] cond_data0;  wire [19:0] cond_data1;
    OBUFT OBUF_dataout0[19:0] (.I(bgrad_data_o),.O(grad_data_i),.T(bgrad_test_i[0]));
    OBUFT OBUF_dataout1[19:0] (.I(bgrad_data_o),.O(grad_data_o),.T(inv_test));
    IBUF IBUF_datain0[19:0] (.I(grad_data_o),.O(cond_data0));
    IBUF IBUF_datain1[19:0] (.I(grad_data_i),.O(cond_data1));
    
    OBUFT OBUF_selout0[2:0] (.I(bgrad_sel_o),.O(grad_sel_i),.T(bgrad_test_i[0]));
    OBUFT OBUF_selout1[2:0] (.I(bgrad_sel_o),.O(grad_sel_o),.T(inv_test));
    IBUF IBUF_selin0[2:0] (.I(grad_sel_o),.O(cond_sel0));
    IBUF IBUF_selin1[2:0] (.I(grad_sel_i),.O(cond_sel1));
    assign bgrad_sel_i = bgrad_test_i[0] ? cond_sel0 : cond_sel1;
    assign bgrad_data_i = bgrad_test_i[0] ? cond_data0 : cond_data1;    
	
// Instantiation of Axi Bus Interface S00_AXI
	SiemensGradient_v1_0_S00_AXI # ( 
		.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
		.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
	) SiemensGradient_v1_0_S00_AXI_inst (
		.GRAD_DATA_I(bgrad_data_i),
        .GRAD_DATA_O(bgrad_data_o),
        .GRAD_SEL_I(bgrad_sel_i),
        .GRAD_SEL_O(bgrad_sel_o),
        .GRAD_CLK_I(bgrad_clk_i),
        .GRAD_CLK_O(bgrad_clk_o),
        .GRAD_TEST_I(bgrad_test_i),
        .GRAD_TEST_O(bgrad_test_o),
		.S_AXI_ACLK(s00_axi_aclk),
		.S_AXI_ARESETN(s00_axi_aresetn),
		.S_AXI_AWADDR(s00_axi_awaddr),
		.S_AXI_AWPROT(s00_axi_awprot),
		.S_AXI_AWVALID(s00_axi_awvalid),
		.S_AXI_AWREADY(s00_axi_awready),
		.S_AXI_WDATA(s00_axi_wdata),
		.S_AXI_WSTRB(s00_axi_wstrb),
		.S_AXI_WVALID(s00_axi_wvalid),
		.S_AXI_WREADY(s00_axi_wready),
		.S_AXI_BRESP(s00_axi_bresp),
		.S_AXI_BVALID(s00_axi_bvalid),
		.S_AXI_BREADY(s00_axi_bready),
		.S_AXI_ARADDR(s00_axi_araddr),
		.S_AXI_ARPROT(s00_axi_arprot),
		.S_AXI_ARVALID(s00_axi_arvalid),
		.S_AXI_ARREADY(s00_axi_arready),
		.S_AXI_RDATA(s00_axi_rdata),
		.S_AXI_RRESP(s00_axi_rresp),
		.S_AXI_RVALID(s00_axi_rvalid),
		.S_AXI_RREADY(s00_axi_rready)
	);

	// Add user logic here

	// User logic ends

	endmodule
