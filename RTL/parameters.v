parameter max_cache_width =  8 ;//256
parameter max_r = 2; //Values from c-model will be restricted to 0 -> 3 for now reference control block for mapping
parameter cur_addr_max = 4; 

`define BLK_SIZE 16
`define BS_SQ `BLK_SIZE*`BLK_SIZE
`define BS_CUBE `BS_SQ*`BLK_SIZE
