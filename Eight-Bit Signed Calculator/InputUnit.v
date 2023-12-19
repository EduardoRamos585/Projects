module InputUnit
(
	input clock,
	input Reset,
	output trig,
	input [3:0] row,
	output [3:0] col,
	output [7:0] B,
	output reg OUT_Range
	
);

 wire [15:0] BCD_Code;
 wire [3:0] value;
 wire [7:0] SignedBinary_Code;
 wire check;
 
 

keypad_input_module keypad_input_module_inst
(
	.clk(clock) ,	// input  clk_sig
	.reset(Reset) ,	// input  reset_sig
	.row(row) ,	// input [3:0] row_sig
	.col(col) ,	// output [3:0] col_sig
	.out(BCD_Code) ,	// output [DIGITS*4-1:0] out_sig
	.value(value) ,	// output [3:0] value_sig
	.trig(trig) 	// output  trig_sig
);

defparam keypad_input_module_inst.DIGITS = 4;






BCDtoSignB BCDtoSignB_inst
(
	.BCD(BCD_Code) ,	// input [15:0] BCD_sig
	.binarySM(SignedBinary_Code) 	// output [N-1:0] binarySM_sig
);

defparam BCDtoSignB_inst.N = 8;



TwosCtoSignM TwosCtoSignM_inst
(
	.A(SignedBinary_Code) ,	// input [7:0] A_sig
	.B(B) 	// output [7:0] B_sig
);

 assign check = B[7];
 
 
 always @(BCD_Code)
	if( BCD_Code > 16'b1110000100100111) begin
		OUT_Range <= 1'b1;
		end
	 else if(BCD_Code[11:0] > 12'b000100100111) begin
		OUT_Range <= 1'b1;
		end
		else begin
	OUT_Range <= 1'b0;
	end

	
	

	 

endmodule 
