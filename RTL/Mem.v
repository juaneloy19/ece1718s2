module Mem #(parameter D_WIDTH = 64, parameter A_WIDTH = 7,parameter A_MAX = 128)
(
	data, 
    read_addr, write_addr,
	we, read_clock, write_clock, 
    q
);
  
    input [D_WIDTH-1:0] data;
    input [A_WIDTH-1:0] read_addr, write_addr;
	input we, read_clock, write_clock;
    output reg [D_WIDTH-1:0] q;
  
	// Declare the RAM variable
    reg [D_WIDTH-1:0] ram[A_MAX-1:0];
	
	always @ (posedge write_clock)
	begin
		// Write
		if (we)
			ram[write_addr] <= data;
	end
	
    //always @ (posedge read_clock)
    always @ (*)
	begin
		// Read 
		q = ram[read_addr];
	end
  
endmodule
