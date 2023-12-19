module Control_Unit
(
	input Enter,
	input Clock,
	input Clear,
	input [3:0] row,
	input SW0,
	output trig,
	output [6:0] HEX3, HEX2, HEX1, HEX0,HEX_PART3,
	output [3:0] col,
	output [3:0] AN,
	output OVR,
	output OUT_RANGE
	
);

wire New_Clock;
wire out;
wire [7:0] B;
wire [7:0] Z;

wire [7:0] ALU_ANS;

reg LOADA, LOADB, IU_AU, LOADOU, Reset, LOADR, RES_IU;




 reg[1:0] state, nextstate;
 parameter S0 = 2'b00, S1 = 2'b01, S2 = 2'b10, S3 = 2'b11;
 
 

 clock_div_module#(32,2500000) clock_div_module_inst
(
	.clk(Clock) ,	// input  clk_sig
	.reset(~Clear) ,	// input  reset_sig
	.clk_out(New_Clock) 	// output  clk_out_sig
);
 

 
 EdgeDetect EdgeDetect_inst
(
	.in(Enter) ,	// input  in_sig
	.clock(New_Clock) ,	// input  clock_sig
	.out(out) 	// output  out_sig
);

 
 
 always @(posedge out, negedge Clear)
		if(Clear == 1'b0) state <= S0;
					else state <= nextstate; 

					

		always @(state,SW0)
				case (state)
								S0:  begin Reset 	= 1'b1; LOADA 	= 1'b0; LOADB 	=1'b0;  LOADR 	= 1'b0; IU_AU 	= 1'b0; LOADOU = 1'b0; RES_IU = 1'b0; nextstate  <= S1; end
								S1:  begin Reset =  1'b0; LOADA = 1'b1; LOADB 	=1'b0;  LOADR 	= 1'b0; IU_AU 	= 1'b0; LOADOU = 1'b1; RES_IU = 1'b0; nextstate  <= S2;  end
								S2:  begin Reset =  1'b0; LOADA 	= 1'b0; LOADB 	= 1'b1; LOADR 	= 1'b0; IU_AU 	= 1'b0; LOADOU = 1'b1;RES_IU = 1'b1; nextstate  <= S3;  end
									  
									  
								S3: if(~SW0) begin Reset =  1'b0; LOADA 	= 1'b0; LOADB 	= 1'b0; LOADR 	= 1'b1; IU_AU 	= 1'b1; LOADOU = 1'b1; RES_IU = 1'b1;  end
									 else begin Reset =  1'b0; LOADA 	= 1'b0; LOADB 	= 1'b0; LOADR 	= 1'b1; IU_AU 	= 1'b1; LOADOU = 1'b1; RES_IU = 1'b1;  end
									 
									 
								
				endcase 
				

	
InputUnit InputUnit_inst
(
	.clock(Clock) ,	// input  clock_sig
	.Reset(~RES_IU) ,	// input  Reset_sig
	.trig(trig) ,	// output  trig_sig
	.row(row) ,	// input [3:0] row_sig
	.col(col) ,	// output [3:0] col_sig
	.B(B) ,	// output [7:0] B_sig
	.OUT_Range(OUT_RANGE) 	// output  OUT_Range_sig
);
				
				
				
				

TwoFunctionALU TwoFunctionALU_inst
(
	.EightBitNum(B) ,	// input [7:0] EightBitNum_sig
	.Reset(~Reset) ,	// input  Reset_sig
	.LoadA(LOADA) ,	// input  LoadA_sig
	.LoadB(LOADB) ,	// input  LoadB_sig
	.ADDSUB(SW0) ,	// input  ADDSUB_sig
	.LoadR(LOADR) ,	// input  LoadR_sig
	.Result_ALU(ALU_ANS) ,	// output [7:0] Result_ALU_sig
	.OVR(OVR) 	// output  OVR_sig
	
);



assign Z =(IU_AU ==0)? B:ALU_ANS;


		
				
OutputUnit OutputUnit_inst
(
	.clock(Clock) ,	// input  clock_sig
	.Reset(~Reset) ,	// input  Reset_sig
	.EightBitNumber(Z) ,	// input [7:0] EightBitNumber_sig
	.AN(AN) ,	// output [3:0] AN_sig
	.HEX0(HEX0) ,	// output [6:0] HEX0_sig
	.HEX1(HEX1) ,	// output [6:0] HEX1_sig
	.HEX2(HEX2) ,	// output [6:0] HEX2_sig
	.HEX3(HEX3) ,	// output [6:0] HEX3_sig
	.HEX_PART3(HEX_PART3) 	// output [6:0] HEX_PART3_sig
);
		
				


		// Debuggin Notes: ALU registers seem not to hold any values, but when Z is used, a result is displayed

				
				
					
				
				
endmodule 
								
								