module OutputUnit
(
	
	input clock, /*ONOff,*/ Reset,
	input [7:0] EightBitNumber,
	output [3:0] AN,
	output [6:0] HEX0,HEX1,HEX2,HEX3,
	output reg [6:0] HEX_PART3
	
);


//wire [7:0] EightBitNumber;
wire [7:0] Rout;
wire [3:0] ones;
wire [3:0] tens;
wire [1:0] hundreds;
wire sign;

wire TEnMHZ, ONEKHz;
wire count;
wire [1:0] Choice;

wire [3:0] Q1, Q2;
wire [1:0] Q3;
wire Q4;
wire [3:0] OUTPUT;
wire [6:0] HEX_Value;

/*
TestPatternGen TestPatternGen_inst
(
	.clock(clock) ,	// input  clock_sig
	.OnOff(ONOff) ,	// input  OnOff_sig
	.Reset(Reset) ,	// input  Reset_sig
	.Test_Pattern(EightBitNumber) 	// output [7:0] Test_Pattern_sig
);

*/

assign sign = ~EightBitNumber[7];

TwosCtoSignM TwosCtoSignM_inst
(
	.A(EightBitNumber) ,	// input [7:0] A_sig
	.B(Rout) 	// output [7:0] B_sig
);







SignMtoBCD SignMtoBCD_inst
(
	.A({1'b0,Rout[6:0]}) ,	// input [7:0] A_sig
	.ONES(ones) ,	// output [3:0] ONES_sig
	.TENS(tens) ,	// output [3:0] TENS_sig
	.HUNDREDS(hundreds) 	// output [1:0] HUNDREDS_sig
);



divideXn #(3'd5, 3'd3) div5
(
	.CLOCK(clock),  // The clock is the input for our division module, which activates the always 
	.CLEAR(Reset),
	.OUT(TEnMHZ),
	.COUNT(count)
);


divideXn #(10'd1000, 4'd10)div1000L
(
	.CLOCK(TEnMHZ),
	.CLEAR(Reset),
	.OUT(ONEKHz),
	.COUNT(count)
	
);


FSM FSM_inst
(
	.clock(ONEKHz) ,	// input  clock_sig
	.reset(Reset) ,	// input  reset_sig
	.RA(Choice) ,	// output [1:0] RA_sig
	.AN(AN) 	// output [3:0] AN_sig
);


NbitRegister NbitRegister_ONES
(
	.D(ones) ,	// input [N-1:0] D_sig
	.Q(Q1) ,	// output [N-1:0] Q_sig
	.CLK(ONEKHz) ,	// input  CLK_sig
	.CLR(Reset) 	// input  CLR_sig
);




NbitRegister NbitRegister_TENS
(
	.D(tens) ,	// input [N-1:0] D_sig
	.Q(Q2) ,	// output [N-1:0] Q_sig
	.CLK(ONEKHz) ,	// input  CLK_sig
	.CLR(Reset) 	// input  CLR_sig
);




NbitRegister NbitRegister_HUNDREDS
(
	.D({2'b00,hundreds}) ,	// input [N-1:0] D_sig
	.Q(Q3) ,	// output [N-1:0] Q_sig
	.CLK(ONEKHz) ,	// input  CLK_sig
	.CLR(Reset) 	// input  CLR_sig
);




NbitRegister NbitRegister_SIGN
(
	.D(sign) ,	// input [N-1:0] D_sig
	.Q(Q4) ,	// output [N-1:0] Q_sig
	.CLK(ONEKHz) ,	// input  CLK_sig
	.CLR(Reset) 	// input  CLR_sig
);





 
 four2one four2one_inst
(
	.A(Choice[0]) ,	// input  A_sig
	.B(Choice[1]),
	.D0(Q1) ,	// input [N-1:0] D0_sig
	.D1(Q2) ,	// input [N-1:0] D1_sig
	.D2(Q3) ,	// input [N-1:0] D2_sig
	.D3({3'b111,Q4}) ,	// input [N-1:0] D3_sig
	.Y(OUTPUT) 	// output [N-1:0] Y_sig
);

defparam four2one_inst.N = 4;

BCDtoSeven HEX_PART3_Sign
(
	.w(OUTPUT[3]) ,	// input  w_sig
	.x(OUTPUT[2]) ,	// input  x_sig
	.y(OUTPUT[1]) ,	// input  y_sig
	.z(OUTPUT[0]) ,	// input  z_sig
	.a(HEX_Value[0]) ,	// output  a_sig
	.b(HEX_Value[1]) ,	// output  b_sig
	.c(HEX_Value[2]) ,	// output  c_sig
	.d(HEX_Value[3]) ,	// output  d_sig
	.e(HEX_Value[4]) ,	// output  e_sig
	.f(HEX_Value[5]) ,	// output  f_sig
	.g(HEX_Value[6]) 	// output  g_sig
);


always @(AN) 
begin 
	HEX_PART3 <= ~(HEX_Value);
end



BCDtoSeven HEX0_ONES
(
	.w(ones[3]) ,	// input  w_sig
	.x(ones[2]) ,	// input  x_sig
	.y(ones[1]) ,	// input  y_sig
	.z(ones[0]) ,	// input  z_sig
	.a(HEX0[0]) ,	// output  a_sig
	.b(HEX0[1]) ,	// output  b_sig
	.c(HEX0[2]) ,	// output  c_sig
	.d(HEX0[3]) ,	// output  d_sig
	.e(HEX0[4]) ,	// output  e_sig
	.f(HEX0[5]) ,	// output  f_sig
	.g(HEX0[6]) 	// output  g_sig
);



BCDtoSeven HEX1_TENS
(
	.w(tens[3]) ,	// input  w_sig
	.x(tens[2]) ,	// input  x_sig
	.y(tens[1]) ,	// input  y_sig
	.z(tens[0]) ,	// input  z_sig
	.a(HEX1[0]) ,	// output  a_sig
	.b(HEX1[1]) ,	// output  b_sig
	.c(HEX1[2]) ,	// output  c_sig
	.d(HEX1[3]) ,	// output  d_sig
	.e(HEX1[4]) ,	// output  e_sig
	.f(HEX1[5]) ,	// output  f_sig
	.g(HEX1[6]) 	// output  g_sig
);



BCDtoSeven HEX2_HUNDREDS
(
	.w(1'b0) ,	// input  w_sig
	.x(1'b0) ,	// input  x_sig
	.y(hundreds[1]) ,	// input  y_sig
	.z(hundreds[0]) ,	// input  z_sig
	.a(HEX2[0]) ,	// output  a_sig
	.b(HEX2[1]) ,	// output  b_sig
	.c(HEX2[2]) ,	// output  c_sig
	.d(HEX2[3]) ,	// output  d_sig
	.e(HEX2[4]) ,	// output  e_sig
	.f(HEX2[5]) ,	// output  f_sig
	.g(HEX2[6]) 	// output  g_sig
);


BCDtoSeven HEX3_SIGN
(
	.w(1'b1) ,	// input  w_sig
	.x(1'b1) ,	// input  x_sig
	.y(1'b1) ,	// input  y_sig
	.z(sign) ,	// input  z_sig
	.a(HEX3[0]) ,	// output  a_sig
	.b(HEX3[1]) ,	// output  b_sig
	.c(HEX3[2]) ,	// output  c_sig
	.d(HEX3[3]) ,	// output  d_sig
	.e(HEX3[4]) ,	// output  e_sig
	.f(HEX3[5]) ,	// output  f_sig
	.g(HEX3[6]) 	// output  g_sig
);





endmodule 
