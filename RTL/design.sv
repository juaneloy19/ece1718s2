`include "Control.v"
`include "Mem.v"
`include "pe.v"

module Me_engine (
  
  //Inputs
  clk,
  reset,
  r,
  go,
  clk_write,
  address_write_ref,
  data_write_ref,
  address_write_cur,
  data_write_cur,
  write_enable_ref,
  write_enable_cur,
  clk_read,
  //Outputs
  m_i,
  m_j,
  done

	);
  
  `include "parameters.v"
  input       clk;
  input       reset;
  input [3:0] r;
  input       go;
  
  input         clk_write;
  input  [6:0]  address_write_ref;
  input  [63:0] data_write_ref;
  input  [4:0]  address_write_cur;
  input  [63:0] data_write_cur;
  input         write_enable_ref;
  input         write_enable_cur;
  input         clk_read;
  output        done;
  output [7:0] m_i;
  output [7:0] m_j;

  wire [63:0] data_read_ref;
  wire [63:0] data_read_cur;
  
  wire  [6:0]  address_read_ref;

  wire  [4:0]  address_read_cur;

  wire [7:0]    p;
  wire [7:0]    p_prime;
  wire [7:0]    c;
  wire          start;

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
    .go(go),
    //Outputs
    .addrCur(address_read_cur),//Hook up to Mem
    .addrRef(address_read_ref),//Hook up to Mem
    .p(p),
    .p_prime(p_prime),
    .c(c),
    .start(start)
  );
  //
  
  //Ayan
   pe_row uPe_row(
    .clk(clk),
    .reset(reset),
    .p(p),
    .p_prime(p_prime),
    .c(c),
    .start(start),
    .done(done),
    .mme(),
    .m_i(m_i),
    .m_j(m_j)
   );
  
  //
  
  
endmodule
