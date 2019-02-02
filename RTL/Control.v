module Control (
  //INPUTS
  clk,
  reset,
  go,
  r,
  rdatCur,
  rdatRef,
  //OUTPUTS
  addrCur,
  addrRef,
  p,
  p_prime,
  c,
  start
);
  
  `include "parameters.v"

  
  input clk;
  input reset;
  input [max_r-1:0] r;
  input go;
  input [63:0] rdatCur;
  input [63:0] rdatRef;
  
  output start;
  output [cur_addr_max-1:0] addrCur;
  output [7:0]              addrRef;
  output [7:0] p;
  output [7:0] p_prime;
  output [7:0] c;
  
  
  parameter IDLE = 4'd0,
  			PRE_START = 4'd1,
  			START = 4'd2,
  			PROCESSING = 4'd3,
  			PRE_DONE = 4'd4,
  			DONE = 4'd5;
  
  
  integer i;
  reg start;
  reg [1:0] index ;
  reg [max_cache_width-1:0]	cache_width_q;
  reg [max_cache_width-1:0]	cache_width;//devided by 8
  reg [max_cache_width-1:0]	stride;
  
  reg [cur_addr_max-1:0] Caddr_cnt;
  reg [max_cache_width-1:0] Raddr_cnt;
  reg [5:0] Raddr_cnt_row;
  reg [12:0]	count;
  wire [12:0]	count_minus16;
  reg [12:0] count_q;

  
  wire [max_cache_width-1:0] cache_height;
  
  
  reg [2:0] cur_state;
  reg [2:0] next_state;
  
  reg [63:0] line_buffer1 [3:0];
  reg [63:0] line_buffer2 [3:0];
 
  reg [63:0] Curline_buffer1 [1:0];
  reg [63:0] Curline_buffer2 [1:0];

  reg switch;

  
  wire update_line1;
  wire update_line2;
  wire update_cur1;
  
  reg [7:0] p_dat1;
  reg [7:0] p_dat2;
  reg [7:0] c_dat1;
  reg [7:0] c_dat2;
  
  assign count_minus16 = count_q-16;
  assign addrCur = Caddr_cnt;
  assign addrRef = Raddr_cnt;
  assign c = count_q[4] ? c_dat1 : c_dat2;
  //assign p = count_q[4] ? p_dat1 : p_dat2;
  assign p = count_q[4] ? p_dat2 : p_dat1;
  assign p_prime = count_q[4] ? p_dat1 : p_dat2;
  //assign p_prime = count_q[4] ? p_dat2 : p_dat1;
  
  assign update_line1 = (count[3:0]<4)&& cur_state!=IDLE && !switch;
  assign update_line2 = (count[3:0]<4)&& cur_state!=IDLE && switch;
  //assign update_cur1 = (count[3:0] <2) && cur_state!=IDLE && (count>=16) && !switch;
  assign update_cur1 = (count[3:0] <2) && cur_state!=IDLE && !switch;
  //assign update_cur2 = (count[3:0] <2) && cur_state!=IDLE && (count>=16) && switch;
  assign update_cur2 = (count[3:0] <2) && cur_state!=IDLE && switch;

  assign cache_height = cache_width;

  assign start=cur_state == START;  
  always @(*) begin
    case(count_q[2:0])
      0:c_dat1 = Curline_buffer1[count_q[3]][7:0];
      1:c_dat1 = Curline_buffer1[count_q[3]][15:8];
      2:c_dat1 = Curline_buffer1[count_q[3]][23:16];
      3:c_dat1 = Curline_buffer1[count_q[3]][31:24];
      4:c_dat1 = Curline_buffer1[count_q[3]][15:8];
      5:c_dat1 = Curline_buffer1[count_q[3]][23:16];
      6:c_dat1 = Curline_buffer1[count_q[3]][31:24];
      7:c_dat1 = Curline_buffer1[count_q[3]][31:24];
  	endcase
  end

  always @(*) begin
    case(count_q[2:0])
      0:c_dat2 = Curline_buffer2[count_q[3]][7:0];
      1:c_dat2 = Curline_buffer2[count_q[3]][15:8];
      2:c_dat2 = Curline_buffer2[count_q[3]][23:16];
      3:c_dat2 = Curline_buffer2[count_q[3]][31:24];
      4:c_dat2 = Curline_buffer2[count_q[3]][15:8];
      5:c_dat2 = Curline_buffer2[count_q[3]][23:16];
      6:c_dat2 = Curline_buffer2[count_q[3]][31:24];
      7:c_dat2 = Curline_buffer2[count_q[3]][31:24];
  	endcase
  end
  
    always @(*) begin
      case(count_q[2:0])
      	0:p_dat1 = line_buffer1[count_q[4:3]][7:0];
      	1:p_dat1 = line_buffer1[count_q[4:3]][15:8];
      	2:p_dat1 = line_buffer1[count_q[4:3]][23:16];
      	3:p_dat1 = line_buffer1[count_q[4:3]][31:24];
      	4:p_dat1 = line_buffer1[count_q[4:3]][39:32];
      	5:p_dat1 = line_buffer1[count_q[4:3]][47:40];
      	6:p_dat1 = line_buffer1[count_q[4:3]][55:48];
      	7:p_dat1 = line_buffer1[count_q[4:3]][63:56];
  	  endcase
  	end
  
    //special logic to make pipeline work

    always@(*) begin
        case(count_q[4:3])
            0:index=2;
            1:index=3;
            2:index=0;
            3:index=1;
        endcase
    end
      always @(*) begin
      case(count_q[2:0])
      	0:p_dat2 = line_buffer2[index][7:0]; //-2 due to specific timing
      	1:p_dat2 = line_buffer2[index][15:8];
      	2:p_dat2 = line_buffer2[index][23:16];
      	3:p_dat2 = line_buffer2[index][31:24];
      	4:p_dat2 = line_buffer2[index][39:32];
      	5:p_dat2 = line_buffer2[index][47:40];
      	6:p_dat2 = line_buffer2[index][55:48];
      	7:p_dat2 = line_buffer2[index][63:56];
  	  endcase
  	end
  
 
  always @ (*) begin
  	case(r) 
    	0: cache_width = 8'd4;//-7 to +8
        1: cache_width = 8'd8;//-15 to +16
    	2: cache_width = 8'd12;//-31 to +32
    	3: cache_width = 8'd16;//-47 to +48
    endcase
  end

  always @ (*) begin
  	case(r) 
    	0: stride = 8'd0;//-7 to +8
        1: stride = 8'd4;//-15 to +16
    	2: stride = 8'd8;//-31 to +32
    	3: stride = 8'd12;//-47 to +48
    endcase
  end
  
  
  always @(posedge clk) begin
    if(reset) begin
      for(i=0; i<4; i=i+1) begin
        line_buffer1[i] <= 64'd0;
        line_buffer2[i] <= 64'd0;
      end
      switch <=0;
    end
    else if(update_line1)
      line_buffer1[count[3:0]] <= rdatRef;
    else if(update_line2)
      line_buffer2[count[3:0]] <= rdatRef;
    else begin
      for(i=0; i<4; i=i+1) begin
          line_buffer1[i] <= line_buffer1[i];
          line_buffer2[i] <= line_buffer2[i];
      end
    end

    if(count[3:0] == 4'd15)
        switch <= !switch;
  end
  
  always @(posedge clk) begin
    if(reset) begin
      for(i=0; i<2; i=i+1) begin
        Curline_buffer1[i] <= 64'd0;
        Curline_buffer2[i] <= 64'd0;
      end
    end
    else if (update_cur1) begin
      Curline_buffer1[count[0]] <= rdatCur;
    end
    else if (update_cur2)begin
      Curline_buffer2[count[0]] <= rdatCur;
    end        
    else begin
      for(i=0; i<2; i=i+1) begin
        Curline_buffer1[i] <= Curline_buffer1[i];
        Curline_buffer2[i] <= Curline_buffer2[i];
      end
    end
  end

  always @(posedge clk) begin
    if(reset) begin
      cur_state <=IDLE;
      cache_width_q<=4'd0;
      count_q<=7'd0;
    end
    else begin
      cur_state <= next_state;
      cache_width_q<=cache_width;
      count_q <= count;
    end
  end
  
  always @(posedge clk) begin
    if(reset || cur_state==DONE) begin
      Caddr_cnt <= 8'd0;
    end
    else if((cur_state==PRE_START||cur_state==START||cur_state==PROCESSING) && (update_cur1||update_cur2)) begin
      Caddr_cnt<=Caddr_cnt + 8'd1;
    end
    else begin
      Caddr_cnt<=Caddr_cnt;
    end
  end
  
  always @(posedge clk) begin
    if(reset || cur_state==DONE) begin
      Raddr_cnt <= 8'd0;
      Raddr_cnt_row <= 8'd0;
      count<=8'd0;
    end
    else if((cur_state==PRE_START || cur_state==START || cur_state==PROCESSING) && (update_line1||update_line2)) begin
        //if(Raddr_cnt[5:0]==6'd63)//Done with one row 
        Raddr_cnt_row<=Raddr_cnt_row+1;
        if(Raddr_cnt_row==6'd63)//Done with one row 
          Raddr_cnt<= (cache_width_q*count[12:8]); //Will need to add offset for bigger sizes
        else if(Raddr_cnt[2:0]==7)
          Raddr_cnt <= Raddr_cnt + stride + 8'd1;
        else
          Raddr_cnt<= Raddr_cnt + 8'd1;
    end
    else begin
      Raddr_cnt<=Raddr_cnt; //Will need to add offset for bigger sizes
    end
      
    if(cur_state==PRE_START || cur_state==START || cur_state==PROCESSING || cur_state==PRE_DONE)
      count <= count + 8'd1;
    else
      count <= 8'd0;
  end
        
  
  always @* begin
    next_state = cur_state;
    case(cur_state)
      IDLE:
      	if(go) begin
        	next_state = PRE_START;
      	end else begin
         	next_state = IDLE;
        end
      PRE_START:
          next_state = START;
      START:
        next_state = PROCESSING;
      PROCESSING:
      	if(count_q==13'd4096)
        	next_state = PRE_DONE;
      	else
          	next_state = PROCESSING;
      PRE_DONE:
        if(count_minus16==13'd4096)
            next_state = DONE;
        else
            next_state=PRE_DONE;
      DONE:
        next_state = IDLE;
    endcase
  end
  
  
endmodule
