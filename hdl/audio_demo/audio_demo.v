// audio_demo.v - module for decoding & filtering microphone data
//
// Description:
// 
// This module instantiates the PDM filter, which does most of the work
// It then synchronizes the signals from the filter's 3MHz clock domain
// And downsamples the 96kHz 'fs_int' signal to 48khz before outputting it as 'data_mic_valid'
//
////////////////////////////////////////////////////////////////////////////////////////////////

module audio_demo (

   /******************************************************************/
   /* Top-level port declarations                                    */
   /******************************************************************/

	input            	clk_i,						// 100MHz system clock from ClockWiz
	input 				clk_6_144MHz,				// 6.144MHz clock from ClockWiz
	input 				clk_locked, 				// active-high flag indicating clock is stable
	input             	rst_i,						// active-high reset for module

	// Connections with board's analog-to-digital converter

	input             	pdm_data_i,					// pulse-density modulated data coming from ADC
	output 	         	pdm_clk_o,					// 3MHz clock needed by the on-board ADC
	output reg         	pdm_lrsel_o,				// tied to 1'b0 for left channel only

	// Connections with FFT & ImgCtrl

	output reg    		data_mic_valid,				// sampling frequency a.k.a. time-domain data enable signal (48kHz)
	output reg 	[15:0]	data_mic);					// decoded time-sample data from PDM filter
				
   /******************************************************************/
   /* Local parameters and variables                                 */
   /******************************************************************/
   
	wire	[15:0]		data_int;

	// signals for dividing sampling frequency to 48kHz

	wire    			fs_int;			// 96kHz sampling frequency from lowpass filter
	wire       			fs_rise;		// indicates rising edge of fs_int
	wire				fs_comb;
	integer     		cnt = 0;

	// signals for 2-stage synchronizer

	reg          		fs_tmp;			// stage 1
	reg					fss_tmp;		// stage 2
				
   /******************************************************************/
   /* PDM instantiation                                              */
   /******************************************************************/

	pdm_filter PDM (

		// Global signals
				
		.clk_i         	(clk_i),               	// I [ 0 ] 100MHz system clock
		.clk_6_144MHz 	(clk_6_144MHz),			// I [ 0 ] 6.144MHz clock from ClockWiz
		.clk_locked		(clk_locked), 			// I [ 0 ] active-high flag indicating clock is stable
		.rst_i         	(rst_i),               	// I [ 0 ] active-high system reset

		// Connections with board's analog-to-digital converter
				
		.pdm_clk_o     (pdm_clk_o),           // O [ 0 ] 3MHz clock needed by the on-board ADC
		.pdm_data_i    (pdm_data_i),          // I [ 0 ] pulse-density modulated data coming from ADC

		// Connections with AudioGen

		.fs_o          (fs_int),              // O [ 0 ]  synchronized before being output as data_mic_valid
		.data_o        (data_int));           // O [15:0] registered before being output as data_mic	
	
   /******************************************************************/
   /* Synchronize signals from PDM                        		     */
   /******************************************************************/
   
   	// 2-stage synchronizer for 3MHz domain signals

	always @(posedge clk_i) begin
		fs_tmp 	<= fs_int;
		fss_tmp <= fs_tmp;
	end

	/******************************************************************/
	/* Local parameters and variables				                  */
	/******************************************************************/
	
	// set 'fs_rise' high on the rising edge of 'fs_int'
	// when it has gone through 1st stage of synchronizer

 	assign 	fs_rise =  fs_tmp ? (fss_tmp ? 1'b0: 1'b1) : (1'b0);

 	assign	fs_comb = cnt ? (fs_rise ? 1'b1 : 1'b0 ) : (1'b0);
	
   /******************************************************************/
   /* Divide the fs by two (48kHz sampling rate)                     */
   /******************************************************************/

	always @ (posedge clk_i) begin

		if (rst_i == 1) begin
			cnt <= 0;
		end

		else if ((fs_rise == 1) && (cnt >= 1)) begin
			cnt <= 0;
		end

		else begin	
			cnt <= cnt + 1;
		end
	end

   /******************************************************************/
   /* Registered outputs						                     */
   /******************************************************************/

	always @ (posedge clk_i) begin
		data_mic_valid 	<= fs_comb;
		data_mic 		<= data_int;
		pdm_lrsel_o 	<= 1'b1;
	end

endmodule