// ChorusBuffer.v --> simple one line description of module
//
// Description:
// ------------
// Wrapper for the 16-bit wide, 65k deep memory block.
//
////////////////////////////////////////////////////////////////////////////////////////////////

module ChorusBuffer #(

	/******************************************************************/
	/* Parameter declarations						                  */
	/******************************************************************/

	// Description of parameter group

	parameter integer 	MEM_WIDTH	=	16,
	parameter integer 	MEM_DEPTH	=	16)			

	/******************************************************************/
	/* Port declarations							                  */
	/******************************************************************/

	(				

	input 							clk, 			// clock signal for Port A & Port B

	input 							wea, 			// write enable for Port A
	input 		[MEM_DEPTH-1:0]		addra,			// address for Port A
	input 		[MEM_WIDTH-1:0] 	dina,			// data input for Port A

	input 		[MEM_DEPTH-1:0] 	addrb,			// address for Port B
	output reg	[MEM_WIDTH-1:0]		doutb); 		// data output for Port B
		
  /******************************************************************/
  /* ChorusBlockRAM instantiation	                                */
  /******************************************************************/
  
	blk_mem_gen_0 ChorusBlockRAM (

		.clka 	(clk),    	// input wire clka
		.wea 	(wea),      // input wire [0 : 0] wea
		.addra 	(addra),  	// input wire [15 : 0] addra
		.dina 	(dina),    	// input wire [15 : 0] dina

		.clkb 	(clk),    	// input wire clkb
		.addrb 	(addrb),  	// input wire [15 : 0] addrb
		.doutb 	(doutb));  	// output wire [15 : 0] doutb

endmodule