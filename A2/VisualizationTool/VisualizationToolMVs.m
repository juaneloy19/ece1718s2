function VisualizationToolMVs(vidInput, mvfile1, mvfile2, num_frames)
    v = VideoReader(vidInput);%VideoReader('ffmpeg-20190219-ff03418-win64-static\output.mp4');
    %v.CurrentTime = 200;
    
    height = v.height/16;
    width = v.width/16;
     
    frame_count =1;
    
    mv1 = parseMVFile(mvfile1,height,width);
    mv2 = parseMVFile(mvfile2,height,width);
    z = absoluteDiffMVs(mv1,mv2,num_frames,height,width);

    fig = figure;
    colormap('hot');
    im = imagesc(cell2mat(z{1,frame_count}));
    brighten(.5);
    colorbar;
    lim = caxis;
    caxis([0 max(cell2mat(z{1,2}(:)))]);

    pt1 = get(gca,'position');
    currAxes = axes;
    set(gca,'position',pt1);
    while (hasFrame(v) & (frame_count <= num_frames)) 
        vidFrame = readFrame(v);
        colormap(gca,'hot');
        im = imagesc(cell2mat(z{1,frame_count}));
        caxis([0 max(cell2mat(z{1,frame_count}(:)))]);
        pt1 = get(gca,'position');
        currAxes = axes;
        set(gca,'position',pt1);
        imv = image(vidFrame, 'Parent', currAxes);
        imv.AlphaData = 0.5;
        currAxes.Visible = 'off';
        pause(1/v.FrameRate);
        frame_count= frame_count +1;
    end
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

function y = absoluteDiffMVs(MV_m1, MV_m2, num_frames, height, width)
    for frame_index = 1:num_frames
        for row_index = 1: height
            for column_index = 1:width
                y{frame_index}{row_index,column_index} = ...
                    sqrt((MV_m2{1,frame_index}{row_index,column_index}(2) - MV_m1{1,frame_index}{row_index,column_index}(2))^2 ...
                    + (MV_m2{1,frame_index}{row_index,column_index}(1) - MV_m1{1,frame_index}{row_index,column_index}(1))^2);
            end
        end
    end
end
    