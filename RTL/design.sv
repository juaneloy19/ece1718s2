`include "Control.v"
`include "Mem.v"

module Me_engine (
  
  //Inputs
  clk,
  reset,
  r,
  clk_write,
  address_write_ref,
  data_write_ref,
  address_write_cur,
  data_write_cur,
  write_enable_ref,
  write_enable_cur,
  clk_read,
  address_read_ref,
  address_read_cur,
  //Outputs
  data_read_ref,
  data_read_cur

	);
  
  `include "parameters.v"
  input       clk;
  input       reset;
  input [3:0] r;
  
  input         clk_write;
  input  [6:0]  address_write_ref;
  input  [63:0] data_write_ref;
  input  [4:0]  address_write_cur;
  input  [63:0] data_write_cur;
  input         write_enable_ref;
  input         write_enable_cur;
  input         clk_read;
  input  [6:0]  address_read_ref;
  input  [4:0]  address_read_cur;
  output [63:0] data_read_ref;
  output [63:0] data_read_cur;
  
  //Dave
  Mem #(.D_WIDTH(64), .A_WIDTH(7), .A_MAX(128)) refMem (
    .data(data_write_ref), 
    .read_addr(address_read_ref), 
    .write_addr(address_write_ref),
    .we(write_enable_ref), 
    .read_clock(clk_read), 
    .write_clock(clk_write), 
    .q(data_read_ref)
  );
  
  Mem #(.D_WIDTH(64), .A_WIDTH(5), .A_MAX(32)) curMem (
    .data(data_write_cur), 
    .read_addr(address_read_cur), 
    .write_addr(address_write_cur),
    .we(write_enable_cur), 
    .read_clock(clk_read), 
    .write_clock(clk_write), 
    .q(data_read_cur)
  );
  
  //
  
  //Juan
  Control uControl(
    //Inputs
    .clk(clk),
    .reset(reset),
    .r(r),
    .rdatCur(data_read_cur),
    .rdatRef(data_read_ref),
    //Outputs
    .addrCur(),//Hook up to Mem
    .addrRef(),//Hook up to Mem
    .p(),
    .p_prime(),
    .c(),
    .start()
  );
  //
  
  //Ayan
  
  //
  
  
endmodule
