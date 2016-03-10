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

    // Microphone signals

    input               micData,                // incoming PDM data from on-board mic -> system
    output              micClk,                 // 3MHz clock signal from system -> on-board mic      
    output              micLRSel);              // outgoing 1'b1 signal to tie mic input to left channel

    /******************************************************************/
    /* Local parameters and variables                                 */
    /******************************************************************/

    wire                clk_100MHz;
    wire                micData_sync;

    reg                 sync_reg1;
    reg                 sync_reg2;
    reg                 sync_reg3;

    /******************************************************************/
    /* Global Assignments                                             */
    /******************************************************************/

    assign AUD_SD = 1'b1;
    assign micLRSel = 1'b1;

    /******************************************************************/
    /* 3-stage synchronizer                                           */
    /******************************************************************/    

    always@(posedge clk_100MHz) begin

        if (!sysreset_n) begin
            sync_reg1 <= 1'b0;
            sync_reg2 <= 1'b0;
            sync_reg3 <= 1'b0;
        end

        else begin
            {sync_reg3, sync_reg2, sync_reg1} <= {sync_reg2, sync_reg1, micData};
        end
    end

    assign micData_sync = sync_reg3;

    /******************************************************************/
    /* EMBSYS instantiation                                           */
    /******************************************************************/

  system EMBSYS (

    // Global signals
    
    .sysclk             (sysclk),
    .sysreset_n         (sysreset_n),
    .clk_100MHz         (clk_100MHz),


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

    // Connections with on-board microphone

    .AUD_PWM            (AUD_PWM),
    .micClk             (micClk),

    // Connections with LEDs

    .led_tri_o          (led),

    // Connections with UART

    .uart_rtl_rxd       (uart_rtl_rxd),
    .uart_rtl_txd       (uart_rtl_txd));

endmodule