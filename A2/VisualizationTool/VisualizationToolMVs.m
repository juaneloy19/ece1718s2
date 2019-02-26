
v = VideoReader('ffmpeg-20190219-ff03418-win64-static\output.mp4');

height = v.height;
width = v.width;
z = zeros(height, width);
for r = 1:height
    for c = 1:width
        z(r,c) = r+c;
    end
end

fig = figure;
colormap('hot');
im = imagesc(z);
colorbar;

pt1 = get(gca,'position');
currAxes = axes;
set(gca,'position',pt1);
while hasFrame(v)
    vidFrame = readFrame(v);
    imv = image(vidFrame, 'Parent', currAxes);
    imv.AlphaData = 0.5;
    currAxes.Visible = 'off';
    pause(1/v.FrameRate);
end