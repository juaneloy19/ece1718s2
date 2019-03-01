function calcQualityMVs(mvfile1, mvfile2, num_frames,height, width)
    mv1 = parseMVFile(mvfile1,height,width);
    mv2 = parseMVFile(mvfile2,height,width);
    [z,meanAbsDiff,sumXdiff,sumYdiff,sumXMv1,sumYMv1,sumXMv2,sumYMv2] = absoluteDiffMVs(mv1,mv2,num_frames,height,width);
    output_str = ['meanAbsDiff = ', num2str(meanAbsDiff)];
    disp(output_str);
    output_str = ['sumXdiff = ', num2str(sumXdiff)];
    disp(output_str);
    output_str = ['sumYdiff = ', num2str(sumYdiff)];
    disp(output_str);
    output_str = ['sumXMv1 = ', num2str(sumXMv1)];
    disp(output_str);
    output_str = ['sumYMv1 = ', num2str(sumYMv1)];
    disp(output_str);
    output_str = ['sumXMv2 = ', num2str(sumXMv2)];
    disp(output_str);
    output_str = ['sumYMv2 = ', num2str(sumYMv2)];
    disp(output_str);
end

function [y,meanAbsdiff,sumXdiff,sumYdiff,sumXMv1,sumYMv1,sumXMv2,sumYMv2] = absoluteDiffMVs(MV_m1, MV_m2, num_frames, height, width)
   
    %Variables to keep track of motion vector sumations
    sumAbsDiff = 0;
    mvIndex = 0;
    sumXdiff = 0;
    sumYdiff = 0;
    sumXMv1 = 0;
    sumYMv1 = 0;
    sumXMv2 = 0;
    sumYMv2 = 0;
   
    for frame_index = 1:num_frames
        for row_index = 1: height
            for column_index = 1:width
                y{frame_index}{row_index,column_index} = ...
                    sqrt((MV_m2{1,frame_index}{row_index,column_index}(2) - MV_m1{1,frame_index}{row_index,column_index}(2))^2 ...
                    + (MV_m2{1,frame_index}{row_index,column_index}(1) - MV_m1{1,frame_index}{row_index,column_index}(1))^2);
                sumAbsDiff = sumAbsDiff + y{frame_index}{row_index,column_index};
                sumXMv1 = sumXMv1 + MV_m1{1,frame_index}{row_index,column_index}(1);
                sumYMv1 = sumYMv1 + MV_m1{1,frame_index}{row_index,column_index}(2);
                sumXMv2 = sumXMv2 + MV_m2{1,frame_index}{row_index,column_index}(1);
                sumYMv2 = sumYMv2 + MV_m2{1,frame_index}{row_index,column_index}(2);
                mvIndex = mvIndex + 1;
            end
        end
    end
    meanAbsdiff = sumAbsDiff/mvIndex;
    sumXdiff = sumXMv2 - sumXMv1;
    sumYdiff = sumYMv2 - sumYMv1;
end

function y = parseMVFile(mvFileName,height,width)
    file = fopen(mvFileName, 'rt');
    while ~feof(file)
         line = fgetl(file);
         if(~isempty(line) & line >= 0)
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