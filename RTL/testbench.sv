// Code your testbench here
// or browse Examples
module tb();
 parameter CLK_PERIOD = 20;
 localparam EOF = -1;
 reg clk, reset, stop_Clock; 
 reg Clock;
 reg [1:0] r;
 reg go;
 integer fd,fd1_status, fd2, fd2_status, fd3, fd3_status, MB_X, MB_Y;
 integer count1, count2, row_count, col_count, rowC_count, colC_count, block_count;
 string data1[3:0];
 string data2[3:0];
 string data3[9:0];
 logic [7:0] data1_pixel [7:0];
 logic [7:0] data2_pixel [7:0];
 logic [15:0] data1_temp;
 logic [15:0] data2_temp;
  
 wire         clk_write;
 reg         done;
 reg  [6:0]  address_write_ref;
 reg  [63:0] data_write_ref;
 reg  [4:0]  address_write_cur;
 reg  [63:0] data_write_cur;
 reg         write_enable_ref;
 reg         write_enable_cur;
 wire         clk_read;

 wire [7:0] m_i;
 wire [7:0] m_j;


 Me_engine DUT (
 .clk(clk),
 .reset(reset),
 .r(r),
 .go(go),
 .clk_write(clk_write),
 .address_write_ref(address_write_ref),
 .data_write_ref(data_write_ref),
 .address_write_cur(address_write_cur),
 .data_write_cur(data_write_cur),
 .write_enable_ref(write_enable_ref),
 .write_enable_cur(write_enable_cur),
 .clk_read(clk_read),
 .m_i(m_i),
 .m_j(m_j),
 .done(done) 
 );

 
 assign clk_write = clk;
 assign clk_read = clk;

  initial begin
  	forever begin
    #2 clk = !clk;
  	end
  end
 
 initial begin
 // Dump waves
 $fsdbDumpfile("test.fsdb");
 $fsdbDumpvars();
 fd = $fopen("p_MEcur_block_rtl_1.txt", "r");
 fd2 = $fopen("p_MEcache_rtl_rtl_1.txt", "r");
 fd3 = $fopen("p_MEmv_rtl_1.txt", "r");
 go=0;
 clk = 0;
 reset = 1; // load first operand
 r = 0;
 block_count=0;
 address_write_ref<=0;
 address_write_cur<=0;
 write_enable_ref<=0;
 write_enable_cur<=0;
 @(posedge clk);
 @(posedge clk);
 reset=0;
 @(posedge clk);
 @(posedge clk);
  
 fork
    begin
        forever begin : Checker
            fd3_status = $fscanf( fd3, "%s %s %s %s %s %s %s %d %s %d", data3[0], data3[1], data3[2], data3[3], data3[4], data3[5], data3[6], MB_Y, data3[8], MB_X);
            @(posedge clk);
            wait(done);
                assert(MB_Y==m_i && MB_X ==m_j) begin
                    $display("Block Matched #%d", block_count);
                    block_count = block_count+1;
                end
                else begin
                    $display("TEST FAILED on block %d", block_count);
                    $finish;
                end
            @(posedge clk);
        end
    end


    begin 
        forever begin : read_current_loop
        fd1_status = $fscanf( fd, "%s %s %s %s", data1[0], data1[1], data1[2], data1[3]);

        rowC_count=0;
        colC_count=0;
        if ( fd1_status == EOF  ) begin
              #1000;
              $display("TEST PASSED");
              $finish;
        end
        else begin
            $display("Loading Current");
            $display("%s %s %s %s", data1[0], data1[1], data1[2], data1[3]);
            for(rowC_count=0; rowC_count < 16; rowC_count=rowC_count+1) begin //32 rows
                for(colC_count =0; colC_count < 16; colC_count = colC_count+8) begin //8 pixels per entry
                    for(count1=0; count1<8; count1=count1+1)begin
                        //fd1_status = $fscanf( fd, "%x", data1_pixel[count1]);
                        fd1_status = $fscanf( fd, "%x", data1_temp);
                        data1_pixel[count1] = data1_temp[7:0];
//                        $display ("value %x", data1_pixel[count1]);
                    end
                    address_write_cur <= colC_count/8 + (rowC_count*2);
                    data_write_cur <= {data1_pixel[7], data1_pixel[6],data1_pixel[5], data1_pixel[4],data1_pixel[3], data1_pixel[2],data1_pixel[1], data1_pixel[0]};
                    write_enable_cur <= 1;
                    @(posedge clk);
               end
            end
       end
        write_enable_cur<=0;
        @(posedge clk);
        $display("DONE Loading Current");
        wait(done);
        @(posedge clk);
     end//For loop
    end //Fork

    begin 
        forever begin : read_reference_loop
        fd2_status = $fscanf( fd2, "%s %s %s %s", data2[0], data2[1], data2[2], data2[3]);
        row_count=0;
        col_count=0;
        if ( fd2_status == EOF  ) begin
              #1000;
              $finish;
        end
        else begin
            $display("Loading Reference");
            $display("%s %s %s %s", data2[0], data2[1], data2[2], data2[3]);
            for(row_count=0; row_count < 32; row_count=row_count+1) begin //32 rows
                for(col_count =0; col_count < 32; col_count = col_count+8) begin //8 pixels per entry
                    for(count2=0; count2<8; count2=count2+1)begin
//                        fd2_status = $fscanf( fd2, "%x", data2_pixel[count2]);
                        fd2_status = $fscanf( fd2, "%x", data2_temp);
                        data2_pixel[count2]= data2_temp[7:0];
//                        $display ("value %x", data2_pixel[count2]);
                    end
                    address_write_ref <= col_count/8 + (row_count*4);
                    data_write_ref <= {data2_pixel[7], data2_pixel[6],data2_pixel[5], data2_pixel[4],data2_pixel[3], data2_pixel[2],data2_pixel[1], data2_pixel[0]};
                    write_enable_ref <= 1;
                    @(posedge clk);
               end
            end
       end
        write_enable_ref<=0;
        go<=1;
        @(posedge clk);
        go<=0;
        @(posedge clk);
        $display("DONE Loading Reference");
        wait(done);
        @(posedge clk);
     end//For loop
    end //Fork
    join
 end//Initial

 
endmodule

