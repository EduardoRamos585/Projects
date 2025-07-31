## Description 
The goal of this project is to construct a serial driver that can interface between the AMD RealDigital Blackboard and an external computer. The method of communication is the UART protocol, which serves as the foundation of this project.

-----------------------------------------------------------------
## Functionality 
This project offers the following functions:

Data Transmitter/Reciver:
* Data sent to the driver is converted into binary form for transmission over a physical UART connection.
* Additional bits are included as dictated by the UART protocol, such as the parity bit, stop bit, and data size bits.
* The same principles are applied for recieving data in the reciever.

```verilog
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

 ```

Baud Rate Generator/Divider:
* To synchronize with other devices that communicate over UART, a baud rate generator is required.
* To achieve a specific baud rate, a divider is used to generate the desired frequency.
* These two work in conjunction to control both transmission and reception.

FIFO:
* A 2-D array comprised of 16 rows with 8 bits in length for each index contains the data of our driver.
* Two pointers—typically referred to as the read and write pointers—are used to manage the data stored in the FIFO buffer.
* Signal flags are included to provide user with extra feedback if necessary.

## How it works
Essentially, we are writing programmable logic that is converted into hardware. This is made possible by the Zynq microchip, which supports FPGA libraries along with custom IP modules.

As a result, we can create our own block design to connect the serial driver to the ARM core integrated within the chip. The following image illustrates this architecture:

<img src="Images/Block Design.png" alt="main" width="800"/>

Essentially, an IP module is a block of programmable language that is synthesized to hardware upon implementation (with the exception of the Arm Cortex-A9). Using an IP is like using a library in other languages, apart from how it operates. In this design, we had the following components: 

ARM Cortex-A9 Processor (processing_sytem_7_0) 

*Provides the processing power of our design such as executing C code, hosting the Linux kernel. 

*Physically built on to the chip. 

Proc_sys_reset_0 (Process System Reset) 

*Provides a synchronous reset signal for multiple blocks. 

Ps7_0_axi_periph (AXI Interconnect) 

*Peripheral bus that allows for cross block communication and interfacing. 

Gpio_0 (General Purpose Input Output Driver) 

*Allows for external pins to be used as input/output. 

Serial_0(Serial Communication Driver) 

*Allows for communication between external PC and Blackboard. 

Once the block design was created, the next step was to develop the individual modules that make up the serial driver. In short, a FIFO buffer, baud rate generator/divider, transmitter, and receiver were constructed for this purpose. As described earlier, each module contributes to the overall functionality of the driver.



