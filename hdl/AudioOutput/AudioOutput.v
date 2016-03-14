// AudioOutput.v --> simple one line description of module
//
// Description:
// ------------
// Give a longer, paragraph description here.
//
////////////////////////////////////////////////////////////////////////////////////////////////

module AudioOutput #(

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
	input 		[15:0]		data_in,			// data read from the DelayBuffer
	input 			 		clk,				// 1.5MHz clock signal
	input 		[1:0] 		sw,

	output reg 				PDM_out, 			// output audio stream going to on-board jack
	output reg	[15:0]		read_address);		// data address to read in DelayBuffer

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
	/* Increment the read address (no overflow)		                  */
	/******************************************************************/

	always @(posedge clk) begin
		
		case (sw[1:0])

		2'b00 : if (bit_index == 4'b1100) begin
					read_address <= read_address + 1'b1;
				end

		2'b01 : if (bit_index == 4'b1101) begin
					read_address <= read_address + 1'b1;
				end

		2'b10 : if (bit_index == 4'b1110) begin
					read_address <= read_address + 1'b1;
				end

		2'b11 : if (bit_index == 4'b1111) begin
					read_address <= read_address + 1'b1;
				end
		endcase
	end

	/******************************************************************/
	/* Take the bit-selection of the DelayBuffer line                 */
	/******************************************************************/

	always @(posedge clk) begin
		PDM_out = data_in[bit_index];
	end
		
endmodule