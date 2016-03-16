// AudioInput.v --> simple one line description of module
//
// Description:
// ------------
// Give a longer, paragraph description here.
//
////////////////////////////////////////////////////////////////////////////////////////////////

module AudioInput #(

	/******************************************************************/
	/* Parameter declarations						                  */
	/******************************************************************/

	// Description of parameter group

	parameter integer 	MEM_WIDTH	=	16,
	parameter integer 	MEM_DEPTH	=	65536)			

	/******************************************************************/
	/* Port declarations							                  */
	/******************************************************************/

	(

	input       [1:0]		sw,
	input 	 				clk,
	input 					PDM_in,

	output reg 	[15:0] 		write_address,
	output reg 		 		write_enable,
	output reg 	[15:0] 		write_data);	

	/******************************************************************/
	/* Local parameters and values		                  	  		  */
	/******************************************************************/

	reg 	[3:0] 	bit_index;

	/******************************************************************/
	/* Increment the bit-index (no overflow)		                  */
	/******************************************************************/

	always @(posedge clk) begin

		bit_index <= bit_index + 1'b1;

	end

	/******************************************************************/
	/* Increment the write address (no overflow)		              */
	/******************************************************************/

	always @(posedge clk) begin
		
		if (bit_index == 4'b1111) begin
			write_address <= write_address + 1'b1;
			write_enable <= 1'b1;
		end

		else begin
			write_address <= write_address;
			write_enable <= 1'b1;
		end

	end

	/******************************************************************/
	/* Write the correct PDM data to each bit 		                  */
	/******************************************************************/

	always @(posedge clk) begin

		write_data[bit_index] <= PDM_in;

	end
		
endmodule