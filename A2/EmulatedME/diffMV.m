function diffMV(mvfile1,mvfile2,diffFile)
    disp("Running diff Motion Vectors");
    fprintf("File 1:%s File 2:%s DiffFile:%s \n",mvfile1,mvfile2,diffFile);
    mvformat = ['(' '%f' ',' '%f' ') '];
    mv1ID = fopen(mvfile1,'r');
    mv2ID = fopen(mvfile2,'r');
    %open diffFile for reading
    diffID = fopen(diffFile,'w');
    
    %loop for all lines
    while (~feof(mv1ID)&&~feof(mv2ID))
        frame_head1 = fgetl(mv1ID);
        disp(frame_head1);
        A1 = parseMV(mv1ID,mvformat,1);
        A1 = reshape(A1,[32,16]);
        A1 = transpose(A1);
        
        frame_head2 = fgetl(mv2ID);
        disp(frame_head2);
        A2 = parseMV(mv2ID,mvformat,1);
        A2 = reshape(A2,[32,16]);
        A2 = transpose(A2);
        
        A3 = A1 - A2;
        disp(A3);
        fprintf(diffID,'%s %s',frame_head1,frame_head2);
        for i=1:1:16
            for j=1:2:32
                fprintf(diffID, '(%f,%f) ', A3(i,j), A3(i,j+1));
            end
        end
        %write A3 to file
    end
end

function vec = parseMV(mvfile,format,index)
    [vec,count] = fscanf(mvfile, format);
    fprintf("Read row %d with %d elements\n", index,count);
end