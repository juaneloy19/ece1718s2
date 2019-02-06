`include "parameters.v"

//PE Implementation
module pe(
 clk,
 reset,
 a,
 b,
 start,
 done,
 me
);
  
  input [7:0] a;
  input [7:0] b;
  input clk;
  input reset;
  input start;
  output [15:0] me;
  output done;  
  
  reg [8:0] q1;
  reg [7:0] q2;
  reg [15:0] accumulator;
  reg q_done_1,q_done_2,q_done_3;
  
  always @(posedge clk) begin
    if(reset) begin
      q1 <= 9'b0;
      q2 <= 8'b0;
      accumulator <= 16'b0;
      q_done_1 <= 1'b0;
      q_done_2 <= 1'b0;
      q_done_3 <= 1'b0;
    end else begin 
      if(start) begin
        // Subtractor
        q1 <= a-b;
      end
      if(q_done_1) begin
        // Absolute value
        if(q1[8]) begin
          q2 <= ~(q1-9'h1);
        end else begin
          q2 <= q1[8:0];
        end
      end
      if(q_done_2) begin
        // Accumulator
        accumulator <= accumulator + q2;
      end
      // Latches for start to done
      q_done_1 <= start;
      q_done_2 <= q_done_1;
      q_done_3 <= q_done_2;
    end     
  end

  // Output
  assign me = accumulator;
  assign done = q_done_3;
endmodule

//----PE ROW----
module pe_row(
  clk,
  reset,
  p,
  p_prime,
  c,
  start,
  done,
  mme,
  m_i,
  m_j);

  input [7:0] p;
  input [7:0] p_prime;
  input [7:0] c;
  input start;
  input reset;
  input clk;
  output done;
  output [7:0] mme;
  output [7:0] m_i;
  output [7:0] m_j;
  
  // 8 bit registers for p
  reg [7:0] q [`BLK_SIZE-1:0];
  // 8 bit registers for p'
  reg [7:0] r [`BLK_SIZE-1:0];
  // Shift register for ME outputs
  reg [15:0] s [`BLK_SIZE-1:0];
  reg s_valid [`BLK_SIZE-1:0];
  reg [7:0] s_mi [`BLK_SIZE-1:0];
  reg [7:0] s_mj [`BLK_SIZE-1:0];  
  reg [15:0] s2 [`BLK_SIZE-1:0]; // Make wire?
  reg [7:0] s2_mi [`BLK_SIZE-1:0]; // Make wire?
  reg [7:0] s2_mj [`BLK_SIZE-1:0]; // Make wire?
  reg q_done [`BLK_SIZE-1:0];
  reg [7:0] in_cycle;
  reg [7:0] out_cycle;
  reg [11:0] done_cycle;
  // Comparator
  reg [15:0] cmp;
  reg [7:0] mi;
  reg [7:0] mj;
  reg [7:0] cmp_mi;
  reg [7:0] cmp_mj;
  reg [1:0] pipe_cycle_delay;
  reg start_out;
  // Initialized, used to determine if initialization is complete
  reg initialized;
  reg pe2s;
  reg q2r;
  reg all_done;
  reg started;
  wire start_init;
  
  // Cycle counters
  always@(posedge clk)begin
    if(reset) begin
      in_cycle <= 8'h0;
      out_cycle <= 8'h0;
      done_cycle <= 12'h0;
      initialized <= 1'b0;
      pipe_cycle_delay <= 2'h0;
      mi <= 7'h0;
      mj <= 7'h0;
      started <= 1'b0;
      q2r <= 1'b0;
      pe2s <= 1'b0;
      start_out <= 1'b0;
      //mme <= 1'b0;
      //m_i <= 1'b0;
      //m_j <= 1'b0;
    end else begin
      if(start) begin
        started <= 1'b1;
      end
      if(start||started) begin
        if(in_cycle<`BLK_SIZE-2) begin
          in_cycle <= in_cycle+1;
          q2r <= 1'b0;
        end else if(in_cycle==`BLK_SIZE-2)begin
          in_cycle <= in_cycle+1;
          q2r <= 1'b1;
        end else begin
          in_cycle <= 8'h0;
          initialized <= 1'b1;
          q2r <= 1'b0;
        end
      end
      if(initialized && !start_out) begin
        pipe_cycle_delay <= pipe_cycle_delay + 1;
        start_out <= pipe_cycle_delay[0];
      end
      if(start_out) begin
        if(out_cycle<`BS_SQ-1)begin
          out_cycle <= out_cycle+1;
          pe2s <= 1'b0;
        end else begin
          out_cycle <= 8'h0;
          pe2s <= 1'b1;
          mi <= mi + 1;
        end
        if(done_cycle<`BS_CUBE-1)begin
          done_cycle <= done_cycle+1;       
        end else begin
          done_cycle <= 12'h0;
          all_done <= 1'b1;
        end
      end
    end
  end
  
  // Overall Integration
  generate
    genvar i;
    for(i = 0; i<`BLK_SIZE-1; i=i+1) begin:g_integrate
      always@(posedge clk) begin
        if(reset==1'b1)begin
          q[i] <= 8'b0;
          r[i] <= 8'b0;
          s[i] <= 15'b0;
          s_mi[i] <= 8'h0;
          s_mj[i] <= 8'h0;
          s_valid[i] <= 1'b0;
        end else if(start||started) begin
          q[i] <= q[i+1];
          // Parallel shifting Q registers into R registers every `BLK_SIZE -1 cycles
          //if(initialized) begin
            if(!q2r)begin
              r[i] <= r[i+1];
            end else begin
              //r[i] <= q[i];
              r[i] <= q[i+1];
            end
          //end else begin
          //  if(q2r)begin
          //    r[i] <= q[i+1];
          //  end
          //end
          // Parallel shifting PE outputs to S registers every `BS_SQ-1 cycles
          if(!pe2s)begin
            s[i] <= s[i+1];
            s_mi[i] <= s_mi[i+1];
            s_mj[i] <= s_mj[i+1];
            s_valid[i] <= s_valid[i+1];
          end else begin
            s[i]    <= s2[i];
            s_mi[i] <= mi;
            s_mj[i] <= mj+i;
            s_valid[i] <= 1'b1;
          end
        end
      end
      pe pe_i(
        .a(c),
        .b(r[i]),
        .clk(clk),
        .reset(reset),
        .start(start_init),
        .me(s2[i]),
        .done(q_done[i])
      );
    end
  endgenerate
    
  always @ (posedge clk) begin
    if(reset==1'b1)begin
      q[`BLK_SIZE-1] <= 8'b0;
      r[`BLK_SIZE-1] <= 8'b0;
      s[`BLK_SIZE-1] <= 15'h0;
      s_mi[`BLK_SIZE-1] <= 8'h0;
      s_mj[`BLK_SIZE-1] <= 8'h0;
      s_valid[`BLK_SIZE-1] <= 1'b0;
    end else if(start||started) begin
      q[`BLK_SIZE-1] <= p;
      //if(initialized) begin
        if(!q2r)begin
          r[`BLK_SIZE-1] <= p_prime;
        end else begin
          // r[`BLK_SIZE-1] <= q[`BLK_SIZE-1];
          r[`BLK_SIZE-1] <= p;
          // p_prime lost during this cycle
          // buffer? or have control hold for one extra cycle?
        end
      //end else begin
      //  if(!q2r)begin
      //    r[`BLK_SIZE-1] <= p_prime;
      //  end else begin
      //    r[`BLK_SIZE-1] <= p;
      //  end
      //end
      if(pe2s)begin
        s[`BLK_SIZE-1] <= s2[`BLK_SIZE-1];
        s_mi[`BLK_SIZE-1] <= mi;
        s_mj[`BLK_SIZE-1] <= `BLK_SIZE/2 -1;
        s_valid[`BLK_SIZE-1] <= 1'b1;
      end else begin
        s_valid[`BLK_SIZE-1] <= 1'b0;
      end
    end
  end
    pe pe_last(
      .a(c),
      .b(r[`BLK_SIZE-1]),
      .clk(clk),
      .reset(reset),
      .start(start_init),
      .me(s2[`BLK_SIZE-1]),
      .done(q_done[`BLK_SIZE-1])
    );
    
  //Comparator
  always @ (posedge clk) begin
    if(reset)begin
      cmp <= 15'h7fff;
      cmp_mi <= 8'h0;
      cmp_mj <= 8'h0;
    end else begin
      // Comparing for minimum value
      if(s_valid[0]&& (cmp>s[0]))begin
        cmp <= s[0]; // add valid signal to s to reduce energy
        cmp_mi <= s_mi[0];
        cmp_mj <= s_mj[0];
      end      
    end
  end
  
  assign start_init = started && initialized;
  assign m_i = cmp_mi;
  assign m_j = cmp_mj;
  assign done = all_done;
    
endmodule
