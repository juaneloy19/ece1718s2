import sys
import cv2
import re
import numpy as np

def initVideoFrame(frame):
	ysp,xsp,chan = frame.shape
	copy = frame
	cv2.rectangle( copy, ( 0,0 ), ( xsp, ysp), ( 0,0,0 ),-1,8)
	return copy

infile = sys.argv[1]
outfile = sys.argv[2]
mvfile = sys.argv[3]
block_size = 16 # Can make input arg
print ("Input: "+infile+" Output: "+outfile+" MV: "+mvfile)

# Create a VideoCapture object and read from input file
# If the input is the camera, pass 0 instead of the video file name
cap = cv2.VideoCapture(infile)
fourcc = cv2.VideoWriter_fourcc('m','p','4','v')
out = cv2.VideoWriter()
mv = open(mvfile,'r')

 
if (cap.isOpened()== False): 
  print("Error opening input video stream or file")
  sys.exit(1)

width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = int(cap.get(cv2.CAP_PROP_FPS))
frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
print ("W = "+str(width)+" H = "+str(height) + " FPS = "+str(fps))
mbw = int(width/block_size)
mbh = int(height/block_size)
o_work = out.open(outfile,fourcc,fps*2,(width,height),True)
num_frames = frames
print("MBH = "+str(mbh)+" MBW = "+str(mbw))

ret,next_frame = cap.read()
ysp,xsp,chan = next_frame.shape
print("XSP: "+str(xsp)+" YSP: "+str(ysp)+" Chan: "+str(chan))
#for frame_num in range (1,frames-1):
for frame_num in range (1,num_frames-1):
	# Get current frame
	print("Frame"+str(frame_num))
	frame = next_frame
	ret,next_frame = cap.read()
	# Parse motion vector file
	# Get frame info
	frame_header = mv.readline()
	print("Frame head: "+frame_header)
	# check that it's not an I frame
	up_frame_valid = np.zeros((ysp,xsp))
	for mvs in range (0,mbh):
		# Get motion vectors
		line = mv.readline()
		line.rstrip("\n")
		print("Row: "+str(mvs))
		#print("Line:"+line)
		nums = re.split('[, \)\(]',line)
		nums.remove('\n')
		nums = list(filter(None,nums))
		nums = list(map(float,nums))
		nums = list(map(int,nums))
		#print(nums)
		print(len(nums))
		mv_x = nums[::2]
		mv_y = nums[1::2]
		print("MVX = "+str(len(mv_x)))
		print("MVY = "+str(len(mv_y)))
		up_frame = frame
		#input("Press Enter to continue...")
		#cv2.imshow('Frame',frame)
		#cv2.waitKey()
		#input("Press Enter to continue...")
		#cv2.imshow('UpFrame',up_frame)
		#cv2.waitKey()
		#cv2.rectangle( up_frame, ( 0,0 ), ( xsp, ysp), ( 0,0,0 ),-1,8)
		# Modify blocks in up converted frame corresponding to this line
		for block in range (0,mbw):
			# Going through each pixel of block
			for j in range (0,block_size):
				jy = (mvs*block_size)+j
				uy = jy+int(mv_y[block]/2)
				for i in range (0,block_size):
					ix = (block*block_size)+i
					ux = ix+int(mv_x[block]/2)
					# Skip cases where index is out of frame
					if((0<=(ux)<xsp)and(0<=(uy)<ysp)):
						up_frame[uy][ux] = frame[jy][ix]
						up_frame_valid[uy][ux]+=1;
		# ending block loop
	# ending block row loop
	# handle blanks and overlaps
	for mvs in range (0,mbh):
		for block in range (0,mbw):
			# Going through each pixel of block
			for j in range (0,block_size):
				jy = (mvs*block_size)+j
				for i in range (0,block_size):
					ix = (block*block_size)+i
					#print("X: "+str(ix)+"Y: "+str(jy))
					if(up_frame_valid[jy][ix] == 0):
						# handle blanks
						#up_frame[jy][ix] = frame[jy][ix]
						up_frame[jy][ix] = [0,0,0]
						#up_frame[i][j] = next_frame[i][j] #try this?
					elif (up_frame_valid[jy][ix] > 1):
						# handle overlaps
						up_frame[jy][ix] = [0,0,0]
						#up_frame[jy][ix] = next_frame[jy][ix]
	#print("Writing current frame")
	#input("Press Enter to continue...")
	#cv2.imshow('Frame',frame)
	#cv2.waitKey()
	out.write(frame)
	#print("Writing rate up frame")
	#input("Press Enter to continue...")
	#cv2.imshow('UPFrame',up_frame)
	#cv2.waitKey()
	out.write(up_frame)
	line = mv.readline()
	#print("Read extra:"+line)
#Write final frame out
out.write(next_frame)

cap.release()
out.release()

oread = cv2.VideoCapture(outfile)
if (oread.isOpened()== False): 
  print("Error opening output video stream or file")
  sys.exit(1)

o_frames = oread.get(cv2.CAP_PROP_FRAME_COUNT)
o_width = int(oread.get(cv2.CAP_PROP_FRAME_WIDTH))
o_height = int(oread.get(cv2.CAP_PROP_FRAME_HEIGHT))
o_fps = int(oread.get(cv2.CAP_PROP_FPS))
print ("Out completed : Frames: " +str(o_frames) + " W = "+str(o_width)+" H = "+str(o_height) + " FPS = "+str(o_fps))

