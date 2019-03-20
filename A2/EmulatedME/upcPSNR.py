import sys
import cv2
import re
import numpy as np

invid = sys.argv[1]
refvid = sys.argv[2]
logfile = sys.argv[3]
block_size = 16 # Can make input arg
print ("Input: "+invid+" Reference: "+refvid+" Logfile: "+logfile)

# Create a VideoCapture object and read from input file
# If the input is the camera, pass 0 instead of the video file name
infile = cv2.VideoCapture(invid)
reffile = cv2.VideoCapture(refvid)
log = open(logfile,'w')

if (infile.isOpened()== False):
	print("Error opening input video stream or file")
	sys.exit(1)

if (reffile.isOpened()== False):
	print("Error opening reference video stream or file")
	sys.exit(1)

width = int(infile.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(infile.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = int(infile.get(cv2.CAP_PROP_FPS))
frames = int(infile.get(cv2.CAP_PROP_FRAME_COUNT))
print ("Input W = "+str(width)+" H = "+str(height) + " FPS = "+str(fps) + " Frames = " + str(frames))
num_frames = 27
#num_frames = int(frames)
width = int(reffile.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(reffile.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = int(reffile.get(cv2.CAP_PROP_FPS))
frames = int(reffile.get(cv2.CAP_PROP_FRAME_COUNT))
print ("Ref W = "+str(width)+" H = "+str(height) + " FPS = "+str(fps) + " Frames = " + str(frames))

if(log.closed == True):
	print("Error opening Mb Type file");
	sys.exit(1);

R = np.square(255)
psnrs = 0
#print("XSP: "+str(xsp)+" YSP: "+str(ysp)+" Chan: "+str(chan))
for frame_num in range (0,num_frames):
	ret,in_frame = infile.read()
	ret,ref_frame = reffile.read()
	inysp,inxsp,inchan = in_frame.shape
	#print("In X: "+str(inxsp)+" Y: "+str(inysp)+" Chan: "+str(inchan))
	refysp,refxsp,refchan = ref_frame.shape
	#print("In X: "+str(refxsp)+" Y: "+str(refysp)+" Chan: "+str(refchan))
	diff = in_frame - ref_frame
	diffs = np.square(diff)
	diffss = np.sum(diffs)/(width*height)
	if(diffss == 0):
		psnr = 0
	else:
		psnr = 10*np.log10(R/diffss)
	psnrs = psnrs + psnr
	print ("Frame : "+str(frame_num)+" PSNR: "+str(psnr))
	log.write("Frame : "+str(frame_num)+" PSNR: "+str(psnr)+"\n")

avg_psnr = psnrs/num_frames
print ("Average PSNR: "+str(avg_psnr))
log.write("Average PSNR: "+str(avg_psnr)+"\n")

infile.release()
reffile.release()