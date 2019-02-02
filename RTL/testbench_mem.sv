// Code your testbench here
// or browse Examples
module tb();
 parameter CLK_PERIOD = 20;
 localparam EOF = -1;
 reg clk, control1, stop_Clock; 
 reg Clock;
 reg [3:0] control2;
 integer fd, fd2, count, count2;
  
 reg         clk_write;
 reg         go;
 reg  [6:0]  address_write_ref;
 reg  [63:0] data_write_ref;
 reg  [4:0]  address_write_cur;
 reg  [63:0] data_write_cur;
 reg         write_enable_ref;
 reg         write_enable_cur;
 reg         clk_read;
 reg  [6:0]  address_read_ref;
 reg  [4:0]  address_read_cur;
 wire [63:0] data_read_ref;
 wire [63:0] data_read_cur;


 Me_engine DUT (
 .clk(clk),
 .reset(control1),
   .r(control2),
 .clk_write(clk_write),
 .address_write_ref(address_write_ref),
 .data_write_ref(data_write_ref),
 .address_write_cur(address_write_cur),
 .data_write_cur(data_write_cur),
 .write_enable_ref(write_enable_ref),
 .write_enable_cur(write_enable_cur),
 .clk_read(clk_read),
 .address_read_ref(address_read_ref),
 .address_read_cur(address_read_cur),
 .data_read_ref(data_read_ref),
 .data_read_cur(data_read_cur)
  
 );
 initial clk = 1'b1;
 
 initial begin
 // Dump waves
 $dumpfile("dump.vcd");
 $dumpvars(1, tb);
 #10 clk = ~clk;
 control1 = 1; // load first operand
 control2 = 3;
 #10 clk = ~clk;
 control1 = 0;
 control2 = 3;      
    
 clk_write = 0;
 clk_read = 0;
 write_enable_ref = 0;
 //write_enable_cur = 0;
 //address_read_cur = 5'h1B;
 address_read_ref= 5'h1B;
 //address_write_cur = address_read_cur;
 address_write_ref = address_read_ref;

  $display("Read initial data.");
  toggle_clk_read;
  $display("data[%0h]: %0h",
    address_read_ref, data_read_ref);
  
  $display("Write new data.");
  write_enable_ref = 1;
  data_write_ref = 8'hC5;
  toggle_clk_write;
  write_enable_ref = 0;
  
  $display("Read new data.");
  toggle_clk_read;
  $display("data[%0h]: %0h",
    address_read_ref, data_read_ref);
 end
 
/* initial
  begin
    Clock = 0;
    stop_Clock = 0;
    while (!stop_Clock)
      #5 Clock = ~Clock;
  end
  
initial
  begin
    fd = $fopen("p_MEcur_block_rtl_1.txt", "r");
    begin : read_loop
      forever @(posedge Clock)
      begin
        count = $fscanf( fd, "%b %b", a, b );
        if ( count == EOF || count == 0 )
          disable read_loop;
        else
          $display("a = %4b, b = %4b", a, b);
      end // forever
    end // read_loop
    
    $fclose( fd );

    stop_Clock = 1;
  end
   */
 task toggle_clk_write;
   begin
     #10 clk_write = ~clk_write;
     #10 clk_write = ~clk_write;
   end
 endtask

 task toggle_clk_read;
   begin
     #10 clk_read = ~clk_read;
     #10 clk_read = ~clk_read;
   end
 endtask
  

endmodule

