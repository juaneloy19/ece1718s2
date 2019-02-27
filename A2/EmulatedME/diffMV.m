function diffMV(mvfile1,mvfile2,diffFile,height,width,block)
    disp("Running diff Motion Vectors");
    fprintf("File 1:%s File 2:%s DiffFile:%s \n",mvfile1,mvfile2,diffFile);
    mvformat = ['(' '%f' ',' '%f' ') '];
    mv1ID = fopen(mvfile1,'r');
    mv2ID = fopen(mvfile2,'r');
    %open diffFile for reading
    diffID = fopen(diffFile,'w');
    mbh = height/block;
    mbw = 2*(width/block);
    fprintf("%d %d %d",mbh,mbw,mbh*mbw);
    
    % String for I frames
    iframe = "pict_type=I";
    
    %loop for all lines
    while (~feof(mv1ID)&&~feof(mv2ID))
        frame_head1 = fgetl(mv1ID);
        disp(frame_head1);
        A1 = parseMV(mv1ID,mvformat,1);

        
        frame_head2 = fgetl(mv2ID);
        disp(frame_head2);
        A2 = parseMV(mv2ID,mvformat,1);
        
        A3 = zeros(mbh,mbw);
        if(~contains(frame_head1,iframe)&&~contains(frame_head2,iframe))
            A1 = reshape(A1,[mbw,mbh]);
            A1 = transpose(A1);
            A2 = reshape(A2,[mbw,mbh]);
            A2 = transpose(A2);
            A3 = A1 - A2;
        end
        
        
        disp(A3);
        fprintf(diffID,'%s\n',frame_head1);
        for i=1:1:mbh
            for j=1:2:mbw
                fprintf(diffID, '(%f,%f) ', A3(i,j), A3(i,j+1));
            end
            fprintf(diffID,'\n');
        end
    end
end

function vec = parseMV(mvfile,format,index)
    [vec,count] = fscanf(mvfile, format);
    fprintf("Read row %d with %d elements\n", index,count);
end