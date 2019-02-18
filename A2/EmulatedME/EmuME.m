%EmuME('output_256x256.mp4', 'output', 'mb', 'grid');
%parse_file('log8x8', 'result.txt', 'result_grid.txt');

function EmuME(videoFile, mpegflowOutput, mb_out, grid_out)
    disp("Running MV_EXTRACTOR");
    command = ['./mpegflow ' videoFile ' --grid8x8' ' >' mpegflowOutput];
    disp(command);
    system(command);
    parse_file(mpegflowOutput, mb_out, grid_out);
    exit;
end

function parse_file(filename, filename1, filename2)
    file = fopen(filename, 'rt');
    outfile1 = fopen(filename1, 'w');
    outfile2 = fopen(filename2, 'w');
    frame_count=0;
    block_count=0;
    while ~feof(file)
        line = fgetl(file);
        header = strsplit(line);
        pic_type = cell2mat(header(4));
        frame_num = cell2mat(header(3));
        size=char(header(6));%Grid size
        shape=strsplit(char(size),'=');
        shape=strsplit(char(shape(2)),'x');
        height = str2double(cell2mat(shape(1)));
        width = str2double(cell2mat(shape(2)));       
        disp(pic_type);%Picture type
        disp(size);
	block_count=0;
        if(strcmp(pic_type, 'pict_type=P'))
            fprintf(outfile1, '%s\n', frame_num);
            fprintf(outfile2, '%s\n', frame_num);
            disp("P-frame");
            MV_X_TABLE=zeros(height/2,width);
            for i=1:height/2%MVX
                MV_X=textscan(file,'%f', width);
                MV_X_TRANS=transpose(MV_X{1,1});
                MV_X_TABLE(i,:)= MV_X_TRANS;
            end
            MV_Y_TABLE=zeros(height/2,width);
            for i=1:height/2%MVY
                MV_Y=textscan(file,'%f', width);
                MV_Y_TRANS=transpose(MV_Y{1,1});
                MV_Y_TABLE(i,:)= MV_Y_TRANS;
            end
            %write to file
            for i=1:height/2
                for j=1:width
                    disp(MV_Y_TABLE(i,j));
                    fprintf(outfile1, 'MB: %f MVY: %f MVX: %f\n', block_count, MV_Y_TABLE(i,j), MV_X_TABLE(i,j));  
                    fprintf(outfile2, '(%f,%f) ', MV_Y_TABLE(i,j), MV_X_TABLE(i,j));
                    block_count=block_count+1;
                end
                fprintf(outfile2, '\n');
            end
            fprintf(outfile1,'\n');
            fprintf(outfile2,'\n');  
            frame_count=frame_count+1;
            line = fgetl(file); %new line charater
        else
            disp('I-frame or Dummy frame');
            for i=1:height
                line = fgets(file);
%                disp(line);
            end
        end
    end
    fclose(outfile1);
    fclose(outfile2);
    fclose(file);
end
