// TwoFunctionALU for final project
module TwoFunctionALU 
(
	input [7:0] EightBitNum,
	input Reset, LoadA, LoadB, ADDSUB, LoadR,
	output [7:0] Result_ALU,
	output [6:0] HEX5, HEX4, HEX3, HEX2, HEX1, HEX0,
	output Cout, OVR
	
	
	
);

	wire [7:0] Aout;
	wire [7:0] Bout;
	wire [7:0] Result;
	wire [7:0] Rout;
	

BasicReg RegisterA
(
	.D(EightBitNum),	// input [7:0] D_sig
	.CLK(LoadA),	// input  CLK_sig
	.CLR(Reset),	// input  CLR_sig
	.Q(Aout) 	// output [7:0] Q_sig
);


BasicReg RegisterB
(
	.D(EightBitNum),	// input [7:0] D_sig
	.CLK(LoadB),	// input  CLK_sig
	.CLR(Reset),	// input  CLR_sig
	.Q(Bout) 	// output [7:0] Q_sig
); 


RippleCarryAdderStructural RippleCarryAdderStructural_inst
(
	.A(Aout) ,	// input [7:0] A_sig
	.B(Bout) ,	// input [7:0] B_sig
	.Cxor(ADDSUB) ,	// input  Cxor_sig
	.S(Result) ,	// output [7:0] S_sig
	.Cout(Cout) ,	// output  Cout_sig
	.OVR(OVR) 	// output  OVR_sig
);




BasicReg RegisterR
(
	.D(Result),	// input [7:0] D_sig
	.CLK(LoadR),	// input  CLK_sig
	.CLR(Reset),	// input  CLR_sig
	.Q(Rout) 	// output [7:0] Q_sig
);

assign Result_ALU = Rout;




binary2seven HEX5A
(
	.w(Aout[7]) ,	// input  w_sig
	.x(Aout[6]) ,	// input  x_sig
	.y(Aout[5]) ,	// input  y_sig
	.z(Aout[4]) ,	// input  z_sig
	.a(HEX5[0]) ,	// output  a_sig
	.b(HEX5[1]) ,	// output  b_sig
	.c(HEX5[2]) ,	// output  c_sig
	.d(HEX5[3]) ,	// output  d_sig
	.e(HEX5[4]) ,	// output  e_sig
	.f(HEX5[5]) ,	// output  f_sig
	.g(HEX5[6]) 	// output  g_sig
);


binary2seven HEX4A
(
	.w(Aout[3]) ,	// input  w_sig
	.x(Aout[2]) ,	// input  x_sig
	.y(Aout[1]) ,	// input  y_sig
	.z(Aout[0]) ,	// input  z_sig
	.a(HEX4[0]) ,	// output  a_sig
	.b(HEX4[1]) ,	// output  b_sig
	.c(HEX4[2]) ,	// output  c_sig
	.d(HEX4[3]) ,	// output  d_sig
	.e(HEX4[4]) ,	// output  e_sig
	.f(HEX4[5]) ,	// output  f_sig
	.g(HEX4[6]) 	// output  g_sig
);

binary2seven HEX3B
(
	.w(Bout[7]) ,	// input  w_sig
	.x(Bout[6]) ,	// input  x_sig
	.y(Bout[5]) ,	// input  y_sig
	.z(Bout[4]) ,	// input  z_sig
	.a(HEX3[0]) ,	// output  a_sig
	.b(HEX3[1]) ,	// output  b_sig
	.c(HEX3[2]) ,	// output  c_sig
	.d(HEX3[3]) ,	// output  d_sig
	.e(HEX3[4]) ,	// output  e_sig
	.f(HEX3[5]) ,	// output  f_sig
	.g(HEX3[6]) 	// output  g_sig
);


binary2seven HEX2B
(
	.w(Bout[3]) ,	// input  w_sig
	.x(Bout[2]) ,	// input  x_sig
	.y(Bout[1]) ,	// input  y_sig
	.z(Bout[0]) ,	// input  z_sig
	.a(HEX2[0]) ,	// output  a_sig
	.b(HEX2[1]) ,	// output  b_sig
	.c(HEX2[2]) ,	// output  c_sig
	.d(HEX2[3]) ,	// output  d_sig
	.e(HEX2[4]) ,	// output  e_sig
	.f(HEX2[5]) ,	// output  f_sig
	.g(HEX2[6]) 	// output  g_sig
);


binary2seven HEX1R
(
	.w(Rout[7]) ,	// input  w_sig
	.x(Rout[6]) ,	// input  x_sig
	.y(Rout[5]) ,	// input  y_sig
	.z(Rout[4]) ,	// input  z_sig
	.a(HEX1[0]) ,	// output  a_sig
	.b(HEX1[1]) ,	// output  b_sig
	.c(HEX1[2]) ,	// output  c_sig
	.d(HEX1[3]) ,	// output  d_sig
	.e(HEX1[4]) ,	// output  e_sig
	.f(HEX1[5]) ,	// output  f_sig
	.g(HEX1[6]) 	// output  g_sig
);



binary2seven HEX0R
(
	.w(Rout[3]) ,	// input  w_sig
	.x(Rout[2]) ,	// input  x_sig
	.y(Rout[1]) ,	// input  y_sig
	.z(Rout[0]) ,	// input  z_sig
	.a(HEX0[0]) ,	// output  a_sig
	.b(HEX0[1]) ,	// output  b_sig
	.c(HEX0[2]) ,	// output  c_sig
	.d(HEX0[3]) ,	// output  d_sig
	.e(HEX0[4]) ,	// output  e_sig
	.f(HEX0[5]) ,	// output  f_sig
	.g(HEX0[6]) 	// output  g_sig
);




endmodule 
