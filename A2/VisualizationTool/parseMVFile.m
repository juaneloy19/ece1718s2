function y = parseMVFile(mvFileName,height,width)
    file = fopen(mvFileName, 'rt');
    while ~feof(file)
         line = fgetl(file);
         if(~isempty(line))
             header = strsplit(line);
             pic_type = cell2mat(header(1));
             frame_num = cell2mat(header(2));
             frame_num = strsplit(frame_num,'=');
             if(pic_type(length(pic_type)) == 'I')
                 y{str2num(cell2mat(frame_num(2)))} = zeroMat(height,width);
                 for skipped_line = 1:height
                        ignored_line = fgetl(file);
                 end
             end
             if(pic_type(length(pic_type)) == 'P')
                 for parsed_line = 1:height
                     line = fgetl(file);
                     mvs_row = strsplit(line, ')');
                     for parsed_block = 1:width
                         mv = strsplit(cell2mat(mvs_row(1,parsed_block)),',');
                         mv_x = cell2mat(mv(1));
                         if(parsed_block == 1)
                            mv_x = mv_x(2:length(mv_x));
                         else
                             mv_x = mv_x(3:length(mv_x));
                         end
                         mv_x = str2double(mv_x);
                         mv_y = str2double(cell2mat(mv(2)));
                         frame_mvs{parsed_line,parsed_block} = [mv_x;mv_y];
                     end
                 end
                 y{str2num(cell2mat(frame_num(2)))} = frame_mvs;
             end
         end
    end
     fclose(file);
end

function x = zeroMat(height,width)
    %x = zeros(height, width);
    for r = 1:height
        for c = 1:width
            x{r,c} = [0,0];
        end
    end
end