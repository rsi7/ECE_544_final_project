// n4fpga.v - Top level module for ECE 544 Final Project
//
//
// Modified by:	Rehan Iqbal
// Organization: Portland State University
//
// Description:
// 
//
// The module assumes that a PmodCLP is plugged into the JA and JB ports,
// and that a PmodENC is plugged into the JD port (bottom row).  
//
//////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps

module n4fpga (

    /******************************************************************/
    /* Top-level port declarations                                    */
    /******************************************************************/

    input               sysclk,                 // 100Mhz clock from on-board oscillator
    input               sysreset_n,             // btnCpuReset from board (active-low reset)

    // Pushbuttons & switches

    input               btnC,                   // center pushbutton
    input               btnU,                   // up (North) pushbutton
    input               btnL,                   // left (West) pushbutton
    input               btnD,                   // down (South) pushbutton
    input               btnR,                   // right (East) pushbutton
    input   [15:0]      sw,                     // Nexys4 on-board slide switches

    // LEDs

    output	[15:0] 		led,			        // Nexys4 on-board LEDs   
    
    // UART serial port (19200 baud)

    input				uart_rtl_rxd,	        // USB UART Rx
    output				uart_rtl_txd,	        // USB UART Tx
    
    // Pmod connectors

    output  [7:0]       PmodCLP_DataBus,        // Connected to Port JA

    output              PmodCLP_RS,             // Connected to JB[4]
    output              PmodCLP_RW,             // Connected to JB[5]
    output              PmodCLP_E,              // Connected to JB[6]

    input               PmodENC_A,              // Connected to JD[5]
    input               PmodENC_B,              // Connected to JD[4]
    input               PmodENC_BTN,            // Connected to JD[6]
    input               PmodENC_SWT,            // Connected to JD[7]

    // Audio output signals to mic jack

    output              AUD_PWM,
    output              AUD_SD,

    // SPI microphone board

    input 				SPI_MISO,
    output 				SPI_SS,
    output 				SPI_CLK,

    // Microphone signals

    input               micData,                // incoming PDM data from on-board mic -> system
    output              micClk,                 // 3MHz clock signal from system -> on-board mic      
    output              micLRSel);              // outgoing 1'b1 signal to tie mic input to left channel

    /******************************************************************/
    /* Local parameters and variables                                 */
    /******************************************************************/

    wire                clk_100MHz;
    wire 				clk_6MHz;
    wire 				clk_3MHz;

    reg                 clk_262kHz_reg;
    wire                clk_262kHz;
    reg     [31:0]      count;

    wire                clk_1MHz;
    wire                clk_1MHz_buff;

    wire    [15:0]      addrb;
    wire    [15:0]      doutb;

    wire                wea;
    wire    [15:0]      addra;
    wire    [15:0]      dina;

    wire                PWM_out;
    wire                micData_sync;

    reg                 mic_sync1;
    reg                 mic_sync2;
    reg                 mic_sync3;

    /******************************************************************/
    /* Global Assignments                                             */
    /******************************************************************/

    assign AUD_SD = 1'b1;
    assign micLRSel = 1'b1;

    assign AUD_PWM = sw[15] ? PDM_out : micData_sync;
    
    /******************************************************************/
    /* 3-stage synchronizer for micData                               */
    /******************************************************************/    

    always@(posedge micClk) begin

        {mic_sync3, mic_sync2, mic_sync1} <= {mic_sync2, mic_sync1, micData};

    end

    assign micData_sync = mic_sync3;

	//******************************************************************/
	//* 3.072MHz Clock Generator							           */
	//******************************************************************/

	// Use built-in BUFR device to divide 6.144MHz clock --> 3.072MHz clock
	// because ClockWiz cannot generate lower than 4MHz

	BUFR  #(

		.BUFR_DIVIDE	(2),
		.SIM_DEVICE		("7SERIES"))

	ClkDivideBy2 (

		.I			(clk_6MHz),
		.CE			(1'b1),
		.CLR 		(1'b0),
		.O			(clk_3MHz));
   
	// Buffering the 3.072MHz clock
	// then send it to the microphone
   
	BUFG ClkDivBuf(

		.I 			(clk_3MHz),
		.O			(micClk));

    //******************************************************************/
    //* 1.536MHz Clock Generator                                       */
    //******************************************************************/

    // Use built-in BUFR device to divide 3.072MHz clock --> 1.536MHz clock
    // because ClockWiz cannot generate lower than 4MHz

/*    BUFR  #(

        .BUFR_DIVIDE    (2),
        .SIM_DEVICE     ("7SERIES"))

    ClkDivideBy2Again (

        .I          (micClk),
        .CE         (1'b1),
        .CLR        (1'b0),
        .O          (clk_1MHz));
   
    // Buffering the 1.536MHz clock
    // then send it to the Audio Output module
   
    BUFG ClkDivBufAgain(

        .I          (clk_1MHz),
        .O          (clk_1MHz_buff));
*/
    /******************************************************************/
    /* EMBSYS instantiation                                           */
    /******************************************************************/

    system EMBSYS (

        // Global signals
        
        .sysclk             (sysclk),
        .sysreset_n         (sysreset_n),
        .clk_100MHz         (clk_100MHz),
        .clk_6MHz           (clk_6MHz),

        // Connections with LCD display

        .PmodCLP_DataBus    (PmodCLP_DataBus),
        .PmodCLP_E          (PmodCLP_E),
        .PmodCLP_RS         (PmodCLP_RS),
        .PmodCLP_RW         (PmodCLP_RW),

        // Connections with rotary encoder

        .PmodENC_A          (PmodENC_A),
        .PmodENC_B          (PmodENC_B),
        .PmodENC_BTN        (PmodENC_BTN),
        .PmodENC_SWT        (PmodENC_SWT),

        // Connections with pushbuttons & switches

        .btnC               (btnC),
        .btnD               (btnD),
        .btnL               (btnL),
        .btnR               (btnR),
        .btnU               (btnU),
        .sw_tri_i           (sw),

        // Connections with LEDs

        .led_tri_o          (led),

        // connections with AudioOutput module

        .addrb              (addrb),
        .clkb               (micClk),
        .doutb              (doutb),

        // connections with AudioInput module

        .clka               (micClk),           // input wire clka
        .wea                (wea),              // input wire [0 : 0] wea
        .addra              (addra),            // input wire [15 : 0] addra
        .dina               (dina),             // input wire [15 : 0] dina
        
        // Connections with UART

        .uart_rtl_rxd       (uart_rtl_rxd),
        .uart_rtl_txd       (uart_rtl_txd));


    /******************************************************************/
    /* AudioOutput instantiation                                      */
    /******************************************************************/

    AudioOutput audiogen (

        .sw             (sw[1:0]),
        .data_in        (doutb),                // data read from the DelayBuffer
        .clk            (micClk),               // 

        .PDM_out        (PDM_out),              // output audio stream going to on-board jack
        .read_address   (addrb));               // data address to read in DelayBuffer

    /******************************************************************/
    /* AudioInput  instantiation                                      */
    /******************************************************************/

    AudioInput audioread (

        .sw             (sw[1:0]),
        .clk            (micClk),               // 3MHz clock
        .PDM_in         (micData_sync),         // synchronized PDM data from on-board mic

        .write_address  (addra),                // data address to read in DelayBuffer
        .write_enable   (wea),
        .write_data     (dina));


endmodule