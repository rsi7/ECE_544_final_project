// n4fpga.v - Top level module for ECE 544 Final Project
//
// Modified by:	Rehan Iqbal
// Organization: Portland State University
//
// Description:
// 
// This is the top-level module for the DSP Audio Player project.
//
// There are three major components: the embedded system (EMBSYS), 
// the AudioInput module, and the AudioOutput module. A 3MHz clock is
// generated and sent to the on-board microphone, which returns PDM
// data to be packaged by AudioInput. From there, it is written to the
// InputBuffer with EMBSYS, processed in the app, then written to
// DelayBuffer (also in EMBSYS). AudioOutput reads this buffer and
// sends a PDM stream out to the on-board audio jack.
//
// PmodENC should be plugged into bottom-row of Port JD.
// Plug the mono audio jack to a powered speaker (low-volume first).
//
// sw[15] controls whether the output is "wet" (delay / chorus)
// or "dry" (no effects applied). It can just pass the PDM audio input
// directly to the audio jack output.
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

    output              AUD_PWM,                // Connected to the on-board Butterworth filter
    output              AUD_SD,                 // Tie high to enable all four stages of the filter

    // Microphone signals

    input               micData,                // incoming PDM data from on-board mic -> system
    output              micClk,                 // 3MHz clock signal from system -> on-board mic      
    output              micLRSel);              // tie high to make mic input on left channel

    /******************************************************************/
    /* Local parameters and variables                                 */
    /******************************************************************/

    wire                clk_100MHz;             // embedded system clock from ClockWiz
    wire 				clk_6MHz;               // secondary clock from ClockWiz
    wire 				clk_3MHz;               // divided clock for use by on-board mic

    wire    [15:0]      addrb;                  // address: connects to Port B on DelayBuffer
    wire    [15:0]      doutb;                  // data output: connects to Port B on DelayBuffer

    wire                wea;                    // write enable: connects to Port A on InputBuffer
    wire    [15:0]      addra;                  // address: connects to Port A on InputBuffer
    wire    [15:0]      dina;                   // data input: connects to Port A on InputBuffer

    wire                micData_sync;           // 3-stage synchronized microphone data

    reg                 mic_sync1;              // stage 1
    reg                 mic_sync2;              // stage 2
    reg                 mic_sync3;              // stage 3

    /******************************************************************/
    /* Global Assignments                                             */
    /******************************************************************/

    assign AUD_SD = 1'b1;                       // tie high for all four on-board filter stages
    assign micLRSel = 1'b1;                     // tie high for mic input on left channel

    assign AUD_PWM = sw[15] ? PDM_out : micData_sync;       // toggle between wet & dry signal being played
    
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

        .sw             (sw[1:0]),              // switch inputs (debugging)
        .data_in        (doutb),                // read data from the DelayBuffer block memory
        .clk            (micClk),               // 3MHz clock shared with on-board mic

        .PDM_out        (PDM_out),              // output PDM stream going to on-board audio jack
        .read_address   (addrb));               // read address for the DelayBuffer block memory

    /******************************************************************/
    /* AudioInput  instantiation                                      */
    /******************************************************************/

    AudioInput audioread (

        .sw             (sw[1:0]),              // switch inputs (debugging)
        .clk            (micClk),               // 3MHz clock shared with on-board mic
        .PDM_in         (micData_sync),         // synchronized PDM data coming from on-board mic

        .write_address  (addra),                // data address to read in DelayBuffer
        .write_enable   (wea),                  // write enable flag for InputBuffer block memory
        .write_data     (dina));                // write data for InputBuffer block memory

endmodule