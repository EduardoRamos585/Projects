
`timescale 1 ns / 1 ps

	module serial_slave_lite_v1_0_AXI #
	(
        // Bit width of S_AXI address bus
        parameter integer C_AXI_DATA_WIDTH	= 32,
        parameter integer C_S_AXI_ADDR_WIDTH = 4 // Should be 4
    )
    (
        // Ports to top level module (what makes this the Serial IP module)
/*
        output wire  TX,
        input wire RX,  // Change registers
        output wire Data_Out,
        output wire intr,*/
        output wire TX,
        input wire RX,
        output wire debugBit,
        output [26:0] Debug,
        output wire Interrupt,


        // AXI clock and reset
        input wire S_AXI_ACLK,
        input wire S_AXI_ARESETN,

        // AXI write channel
        // address:  add, protection, valid, ready
        // data:     data, byte enable strobes, valid, ready
        // response: response, valid, ready
        input wire [C_S_AXI_ADDR_WIDTH-1:0] S_AXI_AWADDR,
        input wire [2:0] S_AXI_AWPROT,
        input wire S_AXI_AWVALID,
        output wire S_AXI_AWREADY,

        input wire [31:0] S_AXI_WDATA,
        input wire [3:0] S_AXI_WSTRB,
        input wire S_AXI_WVALID,
        output wire  S_AXI_WREADY,

        output wire [1:0] S_AXI_BRESP,
        output wire S_AXI_BVALID,
        input wire S_AXI_BREADY,

        // AXI read channel
        // address: add, protection, valid, ready
        // data:    data, resp, valid, ready
        input wire [C_S_AXI_ADDR_WIDTH-1:0] S_AXI_ARADDR,
        input wire [2:0] S_AXI_ARPROT,
        input wire S_AXI_ARVALID,
        output wire S_AXI_ARREADY,

        output wire [31:0] S_AXI_RDATA,
        output wire [1:0] S_AXI_RRESP,
        output wire S_AXI_RVALID,
        input wire S_AXI_RREADY
    );

    // Internal registers
    reg [31:0] latch_data;
    reg [31:0] status;
    reg [31:0] control;
    reg [31:0] BRD;


    //other registers
    wire [8:0] rdDData;

    wire [4:0] tx_WIndex;
    wire [4:0] tx_RIndex;
    wire [4:0] tx_WtrMrk;
    wire [4:0] rx_WIndex;
    wire [4:0] rx_RIndex;
    wire [4:0] rx_WtrMrk;
    reg wrFifo;
    reg rdFifo;
    wire tx_FullB;
    wire tx_EmptyB;
    wire tx_OvfB;
    wire rx_FullB;
    wire rx_EmptyB;
    wire rx_OvfB;
    wire fiEmp;
    wire wr_edge;
    wire rd_edge;
    wire brdSig;
    wire brd16Sig;
    wire rxbrdSig;
    wire brdEdg;
    wire [1:0] plzParity = control[3:2];
    wire [8:0] RXData;
    reg [8:0] rxFifo;
    wire rxWrite;
    wire rxWriteEdg;
    wire AXIRead;
    wire parErr;
    wire FrameErr;






    // Register map
    // ofs  fn
    //   0  data    (r/w)
    //   4  status  (r/w1c)
    //   8  control (r/w)
    //  12  BRD	    (r/w)

    // Register numbers
    localparam integer DATA_REG      = 2'b00;
    localparam integer STATUS_REG    = 2'b01;
    localparam integer CONTROL_REG   = 2'b10;
    localparam integer BRD_REG       = 2'b11;


    // AXI4-lite signals
    reg axi_awready;
    reg axi_wready;
    reg [1:0] axi_bresp;
    reg axi_bvalid;
    reg axi_arready;
    reg [31:0] axi_rdata;
    reg [1:0] axi_rresp;
    reg axi_rvalid;

    // friendly clock, reset, and bus signals from master
    wire axi_clk           = S_AXI_ACLK;
    wire axi_resetn        = S_AXI_ARESETN;
    wire [31:0] axi_awaddr = S_AXI_AWADDR;
    wire axi_awvalid       = S_AXI_AWVALID;
    wire axi_wvalid        = S_AXI_WVALID;
    wire [3:0] axi_wstrb   = S_AXI_WSTRB;
    wire axi_bready        = S_AXI_BREADY;
    wire [31:0] axi_araddr = S_AXI_ARADDR;
    wire axi_arvalid       = S_AXI_ARVALID;
    wire axi_rready        = S_AXI_RREADY;

    // assign bus signals to master to internal reg names
    assign S_AXI_AWREADY = axi_awready;
    assign S_AXI_WREADY  = axi_wready;
    assign S_AXI_BRESP   = axi_bresp;
    assign S_AXI_BVALID  = axi_bvalid;
    assign S_AXI_ARREADY = axi_arready;
    assign S_AXI_RDATA   = axi_rdata;
    assign S_AXI_RRESP   = axi_rresp;
    assign S_AXI_RVALID  = axi_rvalid;

    // Handle gpio input metastability safely
    /*
    reg [31:0] read_port_data;
    reg [31:0] pre_read_port_data;
    always_ff @ (posedge(axi_clk))
    begin
        pre_read_port_data <= gpio_data_in;
        read_port_data <= pre_read_port_data;
    end
    */

    // Assert address ready handshake (axi_awready)
    // - after address is valid (axi_awvalid)
    // - after data is valid (axi_wvalid)
    // - while configured to receive a write (aw_en)
    // De-assert ready (axi_awready)
    // - after write response channel ready handshake received (axi_bready)
    // - after this module sends write response channel valid (axi_bvalid)
    wire wr_add_data_valid = axi_awvalid && axi_wvalid;
    reg aw_en;
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
        begin
            axi_awready <= 1'b0;
            aw_en <= 1'b1;
        end
        else
        begin
            if (wr_add_data_valid && ~axi_awready && aw_en) //First time, we set ready for a short amount of time
            begin
                axi_awready <= 1'b1;
                aw_en <= 1'b0;
            end
            else if (axi_bready && axi_bvalid)
                begin
                    aw_en <= 1'b1;
                    axi_awready <= 1'b0;
                end
            else
                axi_awready <= 1'b0;
        end
    end

    // Capture the write address (axi_awaddr) in the first clock (~axi_awready)
    // - after write address is valid (axi_awvalid)
    // - after write data is valid (axi_wvalid)
    // - while configured to receive a write (aw_en)
    reg [C_S_AXI_ADDR_WIDTH-1:0] waddr;
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
            waddr <= 0;
        else if (wr_add_data_valid && ~axi_awready && aw_en)  // When we are ready, we save the addresss
            waddr <= axi_awaddr;
    end

    // Output write data ready handshake (axi_wready) generation for one clock
    // - after address is valid (axi_awvalid)
    // - after data is valid (axi_wvalid)
    // - while configured to receive a write (aw_en)
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
            axi_wready <= 1'b0;
        else
            axi_wready <= (wr_add_data_valid && ~axi_wready && aw_en); // How we generate a ready signal
    end

    // Write data to internal registers
    // - after address is valid (axi_awvalid)
    // - after write data is valid (axi_wvalid)
    // - after this module asserts ready for address handshake (axi_awready)
    // - after this module asserts ready for data handshake (axi_wready)
    // write correct bytes in 32-bit word based on byte enables (axi_wstrb)
    // int_clear_request write is only active for one clock
    wire wr = wr_add_data_valid && axi_awready && axi_wready;
    integer byte_index;
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
        begin
            latch_data <= 32'b0;
            status <= 32'b0;
            control <= 32'b0;
            BRD <= 32'b0;
        end
        else
        begin
            if (wr)
            begin
                case (axi_awaddr[3:2])  // Should be [3:2]
                    DATA_REG:
                    begin
                        for (byte_index = 0; byte_index <= 3; byte_index = byte_index+1)
                            if ( axi_wstrb[byte_index] == 1)
                            begin
                                latch_data[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
                            end
                        wrFifo <= 1'b1;
                    end
                    STATUS_REG:
                        for (byte_index = 0; byte_index <= 3; byte_index = byte_index+1)
                            if (axi_wstrb[byte_index] == 1)
                                status[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
                    CONTROL_REG:
                        for (byte_index = 0; byte_index <= 3; byte_index = byte_index+1)
                            if (axi_wstrb[byte_index] == 1)
                                control[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
                    BRD_REG:
                        for (byte_index = 0; byte_index <= 3; byte_index = byte_index+1)
                            if (axi_wstrb[byte_index] == 1)
                                BRD[(byte_index*8) +: 8] <= S_AXI_WDATA[(byte_index*8) +: 8];
                endcase
            end
            else
            begin
                   wrFifo <= 1'b0;
                   status <= 32'b0;
            end
        end
    end



    // Send write response (axi_bvalid, axi_bresp)
    // - after address is valid (axi_awvalid)
    // - after write data is valid (axi_wvalid)
    // - after this module asserts ready for address handshake (axi_awready)
    // - after this module asserts ready for data handshake (axi_wready)
    // Clear write response valid (axi_bvalid) after one clock
    wire wr_add_data_ready = axi_awready && axi_wready;
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
        begin
            axi_bvalid  <= 0;
            axi_bresp   <= 2'b0;
        end
        else
        begin
            if (wr_add_data_valid && wr_add_data_ready && ~axi_bvalid)
            begin
                axi_bvalid <= 1'b1;
                axi_bresp  <= 2'b0;
            end
            else if (S_AXI_BREADY && axi_bvalid)
                axi_bvalid <= 1'b0;
        end
    end

    // In the first clock (~axi_arready) that the read address is valid
    // - capture the address (axi_araddr)
    // - output ready (axi_arready) for one clock
    reg [C_S_AXI_ADDR_WIDTH-1:0] raddr;
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
        begin
            axi_arready <= 1'b0;
            raddr <= 32'b0;
        end
        else
        begin
            // if valid, pulse ready (axi_rready) for one clock and save address
            if (axi_arvalid && ~axi_arready)
            begin
                axi_arready <= 1'b1;
                raddr  <= axi_araddr;
            end
            else
                axi_arready <= 1'b0;
        end
    end

    // Update register read data
    // - after this module receives a valid address (axi_arvalid)
    // - after this module asserts ready for address handshake (axi_arready)
    // - before the module asserts the data is valid (~axi_rvalid)
    //   (don't change the data while asserting read data is valid)
    wire rd = axi_arvalid && axi_arready && ~axi_rvalid;
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
        begin
            axi_rdata <= 32'b0;
        end
        else
        begin
            if (rd)
            begin
		// Address decoding for reading registers
		case (raddr[3:2])
		    DATA_REG:
		    begin
		          axi_rdata <= {23'b0,rxFifo}; //Read from bus to the appropiate registers
		          rdFifo <= 1'b1;
		    end
		    STATUS_REG:
		        axi_rdata <= {11'd0,tx_WtrMrk, 3'd0 ,rx_WtrMrk ,parErr,
		           FrameErr, tx_OvfB ,tx_EmptyB ,tx_FullB , rx_OvfB, rx_EmptyB, rx_FullB};
		    CONTROL_REG:
		        axi_rdata <= control;
		    BRD_REG:
			axi_rdata <= BRD;
		endcase
            end
            else
            begin
                   rdFifo <= 1'b0;
            end
        end

    end




    // Assert data is valid for reading (axi_rvalid)
    // - after address is valid (axi_arvalid)
    // - after this module asserts ready for address handshake (axi_arready)
    // De-assert data valid (axi_rvalid)
    // - after master ready handshake is received (axi_rready)
    always_ff @ (posedge axi_clk)
    begin
        if (axi_resetn == 1'b0)
            axi_rvalid <= 1'b0;
        else
        begin
            if (axi_arvalid && axi_arready && ~axi_rvalid)
            begin
                axi_rvalid <= 1'b1;
                axi_rresp <= 2'b0;
            end
            else if (axi_rvalid && axi_rready)
                axi_rvalid <= 1'b0;
        end
    end




    fallingEdge writeEdge(
    .sig(wrFifo),
    .clk(axi_clk),
    .pe(wr_edge)
    );


   risingEdge readEdge(
    .sig(rdFifo),
    .clk(axi_clk),
    .pe(AXIRead)

    );



    assign Debug = {rxFifo,rx_RIndex,rx_WIndex,rx_FullB,rx_EmptyB,rx_OvfB,rx_WtrMrk};
    assign Interrupt = ~rx_EmptyB & control[6];
    //assign status[3] = FullB; // Full bit
    //assign status[4] = EmptyB; //Empty bit
    //assign status [5] = OvfB; // Overflow
    //assign Data_Out = brdSig;


    assign fiEmp = tx_EmptyB;
    assign debugBit = parErr;
    assign probeBrd = brdSig;
    //assign debugBit = EmptyB;
    wire tx_rd_req,tx_rd_req_edge;

    risingEdge txEdge(
    .sig(tx_rd_req),
    .clk(axi_clk),
    .pe(tx_rd_req_edge)
    );

    risingEdge rxEdge(
    .sig(rxWrite),
    .clk(axi_clk),
    .pe(rxWriteEdg)

    );

    FIFO tx_fifo16X9(
    .clk(axi_clk),
    .wrData(latch_data[8:0]),
    .rdData(rdDData),
    .wrRequest(wr_edge),
    .rdRequest(tx_rd_req_edge),
    .reset(~axi_resetn),
    .txOvf(status[5]),
    .wIndex((tx_WIndex)),
    .rIndex((tx_RIndex)),
    .wtrMrk(tx_WtrMrk),
    .fullB(tx_FullB),
    .emptyB(tx_EmptyB),
    .ovfB(tx_OvfB)
    );

     FIFO rx_fifo16X9(
    .clk(axi_clk),
    .wrData(RXData),
    .rdData(rxFifo),
    .wrRequest(rxWriteEdg),
    .rdRequest(AXIRead),
    .reset(~axi_resetn),
    .txOvf(status[2]),
    .wIndex((rx_WIndex)),
    .rIndex((rx_RIndex)),
    .wtrMrk(rx_WtrMrk),
    .fullB(rx_FullB),
    .emptyB(rx_EmptyB),
    .ovfB(rx_OvfB)
    );



    BRD baudrate(
    .clk(axi_clk),
    .test(control[5]),
    .enable(control[4]),
    .BRD(BRD),
    .out(brdSig)
    );


    brdDivider  brd_16(
    .clk(axi_clk),
    .brd(brdSig),
    .brd16(brd16Sig)
    );


     risingEdge BRDEdge(
    .sig(brd16Sig),
    .clk(axi_clk),
    .pe(brdEdg)
    );


    transmitter txModule(
    .reset(~axi_resetn),
    .enable(control[4]),
    .brgen(brdEdg),
    .size(control[1:0]),
    .stop2(control[8]),
    .parity(plzParity),
    .data(rdDData),
    .data_request(tx_rd_req),
    .fifoEmpty(fiEmp),
    .outdata(TX)
    );

    reciever rxModule(
    .clk(axi_clk),
    .brdSense(brdSig),
    .reset(~axi_resetn),
    .RX(RX),
    .enable(control[4]),
    .size(control[1:0]),
    .stop2(control[8]),
    .parity(plzParity),
    .recievdata(RXData),
    .data_request(rxWrite),
    .clrPe(status[7]),
    .clrFe(status[6]),
    .PE(parErr),
    .FE(FrameErr)
    );




endmodule



module BRD(

input clk,
input test,
input enable,
input [31:0] BRD,
output wire out

);


    reg fOut;
    reg [31:0] Counter;
    wire [31:0] matchValue;

    assign matchValue = (BRD >> 1);

    reg [31:0] Target = matchValue;



    always_ff @ (posedge clk)
    begin

          if(enable)
          begin
                Counter <= Counter + 9'b100000000;

                if(Counter[31:8] == Target[31:8])
                begin
                        fOut <= !fOut;

                        Target <= Target + matchValue;
                end
          end

    end

   assign out = fOut && test;


endmodule



module brdDivider(
input clk,
input brd,
output wire brd16

);

   reg brd_old;
   reg [4:0] cntr;
   reg Out;

     always_ff @ (posedge clk)
     begin
          brd_old <= brd;

          if(!brd_old && brd)
          begin
                 cntr <= cntr + 1'b1;

                 if(cntr == 5'b01111)
                 begin
                       Out <= 1'b1;
                       cntr <= 5'b00000;
                 end
                 else
                 begin
                       Out <= 1'b0;
                 end

          end
     end

     assign brd16 = Out;


endmodule


module transmitter(
input reset,
input enable,
input brgen,
input [1:0] size,
input stop2,
input wire [1:0] parity,
input [8:0] data,
output wire data_request,
input fifoEmpty,
output wire outdata
);

   localparam integer IDLE =    4'b0000;
   localparam integer START =   4'b0001;
   localparam integer TXD0 =    4'b0010;
   localparam integer TXD1 =    4'b0011;
   localparam integer TXD2 =    4'b0100;
   localparam integer TXD3 =    4'b0101;
   localparam integer TXD4 =    4'b0110;
   localparam integer TXD5 =    4'b0111;
   localparam integer TXD6 =    4'b1000;
   localparam integer TXD7 =    4'b1001;
   localparam integer PARITY =  4'b1010;
   localparam integer STOP1 =   4'b1011;
   localparam integer STOP2 =   4'b1100;

   reg [3:0] state = IDLE;
   reg outBit;
   reg Parity;
   reg incRdindex;




   assign outdata = outBit;
   assign data_request = incRdindex;


   always_ff @(posedge brgen)
   begin
        if(enable)
        begin


                 if(reset)
                 begin
                       state <= IDLE;
                       outBit <= 1'b1;

                 end
                 else if((state == IDLE) && (fifoEmpty))
                 begin
                       incRdindex <= 1'b0;
                       state <= IDLE;
                       outBit <= 1'b1;
                       Parity <= 1'b0;
                 end
                 else if((state == IDLE) && (!fifoEmpty))
                 begin
                       incRdindex <= 1'b0;
                       state <= START;
                       outBit <= 1'b0;
                 end
                 else if((state ==  START))
                 begin
                        state <= TXD0;
                        outBit <= data[0];
                        Parity <= ^data[7:0];
                 end
                 else if((state == TXD0))
                 begin
                        state <= TXD1;
                        outBit <= data[1];
                 end
                 else if((state == TXD1))
                 begin
                        state <= TXD2;
                        outBit <= data[2];
                 end
                 else if((state == TXD2))
                 begin
                        state <= TXD3;
                        outBit <= data[3];
                 end
                 else if((state == TXD3))
                 begin
                        state <= TXD4;
                        outBit <= data[4];
                 end
                 else if((state == TXD4))
                 begin
                         if((size == 2'b00) && (parity == 2'b00)) // Length of 5 and no parity
                         begin

                              state <= STOP1;

                              if(stop2)
                              begin
                                    state <= STOP2;
                                    outBit <= 1'b0;
                              end
                              else
                              begin
                                    state <= IDLE;
                                    outBit <= 1'b1;
                                    incRdindex <= 1'b1;
                             end

                         end
                         else if((size == 2'b00) && (parity != 2'b00)) // Length of 5 and parity is needed
                         begin
                                state <= PARITY;

                                if((parity == 2'b01)) // If even parity & num of 1 is even
                                begin
                                       if(!Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end


                                end
                                if((parity == 2'b10)) // If odd parity & num of 1 is odd
                                begin
                                       if(Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end
                                end

                         end
                         else
                         begin
                                state <= TXD5;    //It was bigger than 5
                                outBit <= data[5];

                         end

                 end
                 else if((state == TXD5))
                 begin
                         if((size == 2'b01) && (parity == 2'b00)) // Length of 6  and no parity
                         begin
                                state <= STOP1;
                                if(stop2)
                                begin
                                    state <= STOP2;
                                    outBit <= 1'b0;
                                end
                                else
                                begin
                                    state <= IDLE;
                                    outBit <= 1'b1;
                                    incRdindex <= 1'b1;
                                end
                         end
                         else if((size == 2'b01) && (parity != 2'b00))
                         begin
                                state <= PARITY;

                                 if((parity == 2'b01)) // If even parity & num of 1 is even
                                begin
                                       if(!Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end

                                end
                                if((parity == 2'b10)) // If odd parity & num of 1 is odd
                                begin
                                       if(Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end
                                end

                         end
                         else
                         begin
                                state <= TXD6; //It was bigger than 6
                                outBit <= data[6];

                         end

                 end
                 else if((state == TXD6))
                 begin
                        if((size == 2'b10) && (parity == 2'b00))
                        begin
                               state <= STOP1;
                               if(stop2)
                               begin
                                     state <= STOP2;
                                     outBit <= 1'b0;
                               end
                               else
                               begin
                                     state <= IDLE;
                                     outBit <= 1'b1;
                                      incRdindex <= 1'b1;
                                 end
                        end
                        else if((size == 2'b10) && (parity != 2'b00))
                        begin
                               state <= PARITY;

                                if((parity == 2'b01)) // If even parity & num of 1 is even
                                begin
                                       if(!Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end
                                end
                                if((parity == 2'b10)) // If odd parity & num of 1 is odd
                                begin
                                       if(Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end
                                end
                        end
                        else
                        begin
                               state <= TXD7;
                               outBit <= data[7];

                        end

                 end
                 else if((state == TXD7))
                 begin
                        if((size == 2'b11) && (parity == 2'b00))
                        begin
                                state <= STOP1;
                                if(stop2)
                                begin
                                     state <= STOP2;
                                     outBit <= 1'b0;
                                end
                                else
                                begin
                                     state <= IDLE;
                                     outBit <= 1'b1;
                                     incRdindex <= 1'b1;
                                end
                        end
                        else if((size == 2'b11) && (parity != 2'b00))
                        begin
                                state <= PARITY;

                                 if((parity == 2'b01)) // If even parity & num of 1 is even
                                begin
                                       if(!Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end
                                end
                                if((parity == 2'b10)) // If odd parity & num of 1 is odd
                                begin
                                       if(Parity)
                                       begin
                                             outBit <= 1'b0;
                                       end
                                       else
                                       begin
                                             outBit <= 1'b1;
                                       end
                                end
                                else if(parity == 2'b11)
                                begin
                                       outBit <= data[8];
                                end
                        end

                 end
                 else if((state == PARITY))
                 begin
                        state <= STOP1;
                        if(stop2)
                        begin
                              state <= STOP2;
                              outBit <= 1'b1;
                       end
                       else
                       begin
                              state <= IDLE;
                              outBit <= 1'b1;
                              incRdindex <= 1'b1;
                      end

                 end
                 else if((state == STOP2))
                 begin
                        state <= IDLE;
                        outBit <= 1'b1;
                        incRdindex <= 1'b1;
                 end
          end
   end

endmodule

module reciever(
input clk,
input wire brdSense,
input reset,
input RX,
input enable,
input [1:0] size,
input stop2,
input wire [1:0] parity,
output wire [8:0] recievdata,
output wire data_request,
input wire clrPe,
input wire clrFe,
output wire PE,
output wire FE
);

    localparam integer IDLE =    4'b0000;
    localparam integer START =   4'b0001;
    localparam integer RXD0 =    4'b0010;
    localparam integer RXD1 =    4'b0011;
    localparam integer RXD2 =    4'b0100;
    localparam integer RXD3 =    4'b0101;
    localparam integer RXD4 =    4'b0110;
    localparam integer RXD5 =    4'b0111;
    localparam integer RXD6 =    4'b1000;
    localparam integer RXD7 =    4'b1001;
    localparam integer PARITY =  4'b1010;
    localparam integer STOP1 =   4'b1011;
    localparam integer STOP2 =   4'b1100;


    reg brdOld;
    reg latch_Rx;
    reg trigger;
    reg [3:0] state = IDLE;
    reg [4:0] Count;
    reg check1;
    reg check2;
    reg check3;
    reg [1:0] comBit;
    reg [8:0] rxData;
    reg checkParity = 1'b0;
    reg parityErr = 1'b0;
    reg frameErr = 1'b0;
    reg incWr;

    assign recievdata = rxData;
    assign data_request = incWr;
    assign PE = parityErr;
    assign FE = frameErr;


     always_ff @ (posedge clk)
     begin
           if(enable)
           begin
                       latch_Rx <= RX;
                       brdOld <= brdSense;
                       trigger <= brdSense & ~brdOld;


                       if(reset)
                       begin
                            state <= IDLE;
                            Count <= 1'b0;
                            checkParity <= 1'b0;
                            comBit <= 2'b00;
                            //incWr <= 1'b0;
                            //rxData <= 9'd0;
                       end



                       if((state == IDLE))
                       begin
                              Count <= 5'b0;
                              incWr <= 1'b0;
                              checkParity <= 1'b0;
                              comBit <= 2'b00;
                              check1 <= 1'b0;
                              check2 <= 1'b0;
                              check3 <= 1'b0;

                              if((clrPe) && (parityErr))
                              begin
                                    parityErr <= 1'b0;
                              end

                              if((clrFe) && (frameErr))
                              begin
                                    frameErr <= 1'b0;
                              end

                              if((latch_Rx == 1'b0))
                                begin
                                    state <= START;
                                end
                       end
                       else if((state == START) && (trigger))
                       begin

                            Count <= Count + 1'b1;
                            rxData <= 9'd0;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((comBit[1] == 1'b0) && (Count == 5'd15))
                            begin
                                    state <= RXD0;
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;
                            end
                            else if(Count == 5'd15)
                            begin
                                   state <= IDLE;
                                   Count <= 5'b00000;
                                   comBit <= 2'b00;

                            end
                       end
                       else if((state == RXD0) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                    state <= RXD1;
                                    rxData[0] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;
                            end
                       end
                       else if((state == RXD1) && (trigger) )
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                    state <= RXD2;
                                    rxData[1] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;

                            end
                       end
                       else if((state == RXD2) && (trigger) )
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                    state <= RXD3;
                                    rxData[2] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;
                            end
                       end
                       else if((state == RXD3) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                    state <= RXD4;
                                    rxData[3] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;
                            end
                       end
                       else if((state == RXD4) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'b01111) && (size != 2'b00))
                            begin
                                    state <= RXD5;
                                    rxData[4] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;

                            end
                            else if(Count == 5'd15)
                            begin
                                    if(parity != 2'b00)
                                    begin
                                          state <= PARITY;
                                          rxData[4] <= comBit[1];
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                    end
                                    else
                                    begin
                                          state <= STOP1;
                                          rxData[4] <= comBit[1];
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                    end

                            end
                       end
                       else if((state == RXD5) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15) && (size != 2'b01))
                            begin
                                    state <= RXD6;
                                    rxData[5] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;

                            end
                            else if(Count == 5'd15)
                            begin
                                    if(parity != 2'b00)
                                    begin
                                          state <= PARITY;
                                          rxData[5] <= comBit[1];
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                    end
                                    else
                                    begin
                                          state <= STOP1;
                                          rxData[5] <= comBit[1];
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                    end
                            end
                       end
                       else if((state == RXD6) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15) && (size != 2'b10))
                            begin
                                    state <= RXD7;
                                    rxData[6] <= comBit[1];
                                    Count <= 5'b00000;
                                    comBit <= 2'b00;

                            end
                            else if(Count == 5'd15)
                            begin
                                    if(parity != 2'b00)
                                    begin
                                          state <= PARITY;
                                          rxData[6] <= comBit[1];
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                    end
                                    else
                                    begin
                                          state <= STOP1;
                                          rxData[6] <= comBit[1];
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                    end
                            end
                       end
                       else if((state == RXD7) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                 if(parity != 2'b00)
                                 begin
                                      state <= PARITY;
                                      rxData[7] <= comBit[1];
                                      Count <= 5'b00000;
                                      comBit <= 2'b00;
                                 end
                                 else
                                 begin
                                      state <= STOP1;
                                      rxData[7] <= comBit[1];
                                      Count <= 5'b00000;
                                 end
                            end
                       end
                       else if((state == PARITY) && (trigger))
                       begin
                            checkParity <= ^rxData[7:0];
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end


                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                  if((parity - 1'b1) != (checkParity ^ comBit[1]))
                                  begin
                                          parityErr <= 1'b1;
                                          state <= STOP1;
                                          Count <= 5'b00000;
                                          comBit <= 2'b00;

                                  end
                                  else if(parity == 2'b11)
                                  begin
                                        rxData[8] <= comBit[1];
                                        state <= STOP1;
                                        //parityErr <= 1'b0;
                                        Count <= 5'b00000;
                                        comBit <= 2'b00;

                                 end
                                 else
                                 begin
                                       state <= STOP1;
                                       //parityErr <= 1'b0;
                                       Count <= 5'b00000;
                                       comBit <= 2'b00;
                                 end

                            end

                       end
                       else if((state == STOP1) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end

                            comBit <= check1 + check2 + check3;

                            if((Count == 5'd15))
                            begin
                                   if(stop2)
                                   begin
                                          if(comBit[1] != 1'b1)
                                          begin
                                                //Frame Error
                                                incWr <= 1'b1;
                                                frameErr <= 1'b1;
                                                state <= STOP2;
                                                Count <= 5'b00000;
                                                comBit <= 2'b00;

                                          end
                                          else
                                          begin
                                                incWr <= 1'b1;
                                                state <= STOP2;
                                                Count <= 5'b00000;
                                                comBit <= 2'b00;
                                          end
                                   end
                                   else
                                   begin
                                          if(comBit[1] != 1'b1)
                                          begin
                                                //Frame Error
                                                frameErr <= 1'b1;
                                                incWr <= 1'b1;
                                                state <= IDLE;
                                                Count <= 5'b00000;
                                                comBit <= 2'b00;

                                          end
                                          else
                                          begin
                                                incWr <= 1'b1;
                                                state <= IDLE;
                                                Count <= 5'b00000;
                                                comBit <= 2'b00;
                                          end

                                   end

                            end




                       end
                       else if((state == STOP2) && (trigger))
                       begin
                            Count <= Count + 1'b1;

                            if(Count == 5'b00111)
                            begin
                                  check1 <= latch_Rx;
                            end

                            if(Count == 5'b01000)
                            begin
                                  check2 <= latch_Rx;
                            end

                            if(Count == 5'b01001)
                            begin
                                  check3 <= latch_Rx;
                            end
                            comBit <= check1 + check2 + check3;
                            if((Count == 5'd15))
                            begin
                                   if(comBit[1] != 1'b1)
                                   begin
                                        //Frame Error
                                        frameErr <= 1'b1;
                                        state <= IDLE;
                                        incWr <= 1'b1;
                                        Count <= 5'b00000;
                                        comBit <= 2'b00;

                                   end
                                   else
                                   begin
                                        state <= IDLE;
                                        incWr <= 1'b1;
                                        Count <= 5'b00000;
                                        comBit <= 2'b00;
                                   end

                            end


                       end
             end
     end


endmodule


module FIFO(

input clk,
input reg [8:0] wrData,
output wire [8:0] rdData,
input  wrRequest,
input  rdRequest,
input reset,
input txOvf,

output  [4:0] wIndex,
output  [4:0] rIndex,
output  [4:0] wtrMrk,
output fullB,
output emptyB,
output ovfB

);


    reg [15:0][8:0] FIFO;
    reg [4:0] wrIndex;
    reg [4:0] rdIndex;
    reg [4:0] wtrMark;
    reg ovfBit;
   // reg data_Ready;
  // reg run_Once;


    //Logic goes here



    wire full = (wrIndex[4] != rdIndex[4]) && (wrIndex[3:0] == rdIndex[3:0]);
    wire empty = wrIndex == rdIndex;
    assign wIndex = wrIndex;
    assign rIndex = rdIndex;
    assign wtrMrk = wtrMark;
    assign fullB = full;
    assign emptyB = empty;
    assign ovfB = ovfBit;

    assign rdData = FIFO[rdIndex[3:0]];



    always_ff @(posedge clk)
    begin
        if(reset)
          begin
              wrIndex <= 5'b0;
              rdIndex <= 5'b0;
              wtrMark <= 5'b0;
              ovfBit <= 1'b0;
          end
          else
          begin
               if(txOvf)
               begin
                   ovfBit <= 1'b0;
               end


               if(!full && wrRequest)
               begin
                    FIFO[wrIndex[3:0]] <= wrData;
                    wrIndex <= wrIndex + 1'b1;

               end

               if(full && wrRequest)
               begin
                     ovfBit <= 1'b1;
               end

               if(rdRequest && !empty)
               begin
                     rdIndex <= rdIndex + 1'b1;
               end

                wtrMark <= wrIndex - rdIndex;


          end
    end




endmodule

module fallingEdge(
    input sig,
    input clk,
    output pe
    );


    reg prev_sig;



    always_ff @(posedge clk)
    begin
          prev_sig <= sig;
    end

    assign pe = ~sig & prev_sig;



endmodule


module risingEdge(
    input sig,
    input clk,
    output pe
    );


    reg prev_sig, double_prev_sig;



    always_ff @(posedge clk)
    begin
          prev_sig <= sig;
          double_prev_sig <= prev_sig;
    end

    assign pe = prev_sig & ~double_prev_sig;



endmodule





