import sys
import cv2
import re
import numpy as np

infile = sys.argv[1]
outfile = sys.argv[2]
mvfile = sys.argv[3]
mbfile = sys.argv[4]
block_size = 16 # Can make input arg
print ("Input: "+infile+" Output: "+outfile+" MV: "+mvfile)

# Create a VideoCapture object and read from input file
# If the input is the camera, pass 0 instead of the video file name
cap = cv2.VideoCapture(infile)
fourcc = cv2.VideoWriter_fourcc('m','p','4','v')
out = cv2.VideoWriter()
mv = open(mvfile,'r')
mb = open(mbfile,'r')

 
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
#num_frames = 4
num_frames = int(frames/4)
print("MBH = "+str(mbh)+" MBW = "+str(mbw))

if(mb.closed == True):
	print("Error opening Mb Type file");
	sys.exit(1);
if(mv.close == True):
	print("Error opening Mv Type file");
	sys.exit(1);

# Prepare to parse Mb Type file
frame_line_pattern = re.compile("^\[h264 @.*\] ");
lib_line_pattern = re.compile("^\[libx264 @.*\] ");
frame_head_pattern = "New frame";

ret,next_frame = cap.read()
ysp,xsp,chan = next_frame.shape
print("XSP: "+str(xsp)+" YSP: "+str(ysp)+" Chan: "+str(chan))
#for frame_num in range (1,frames-1):
for frame_num in range (1,num_frames-1):
	# Get current frame
	print("Frame"+str(frame_num))
	frame = next_frame.copy()
	out.write(frame)
	ret,next_frame = cap.read()
	# Parse motion vector file
	# Get frame info
	frame_header = mv.readline()
	print("Frame head: "+frame_header)
	mb_line = mb.readline();
	#if(frame_head_pattern not in mb_line): mb_line = mb.readline()
	print("Frame Mb head: "+mb_line)
	# check that it's not an I frame
	up_frame_valid = np.zeros((ysp,xsp))
	for mvs in range (0,mbh):
		# Get motion vectors
		line = mv.readline()
		line.rstrip("\n")
		#print("Row: "+str(mvs))
		#print("Line:"+line)
		nums = re.split('[, \)\(]',line)
		nums.remove('\n')
		nums = list(filter(None,nums))
		nums = list(map(float,nums))
		nums = list(map(int,nums))
		#print(nums)
		#print(len(nums))
		mv_x = nums[1::2]
		mv_y = nums[::2]
		#print("MVX = "+str(len(mv_x)))
		#print("MVY = "+str(len(mv_y)))
		up_frame = frame.copy()
		# Get Mb types
		mb_line = mb.readline()
		#if( not frame_line_pattern.match(mb_line)): mb.readline();
		mb_info = re.sub(frame_line_pattern,'',mb_line);
		mb_types = re.findall('...',mb_info);
		#print (str(mvs)+" : "+ str(len(mb_types)));
		# Modify blocks in up converted frame corresponding to this line
		for block in range (0,mbw):
			# Going through each pixel of block
			for j in range (0,block_size):
				jy = (mvs*block_size)+j
				uy = jy+int(mv_y[block]/2)
				#curr_mb = mb_types[block]
				#print(mb_types[block])
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
			curr_mb = mb_types[block][0]
			for j in range (0,block_size):
				jy = (mvs*block_size)+j
				uy = jy-int(mv_y[block]/2)
				for i in range (0,block_size):
					ix = (block*block_size)+i
					ux = ix-int(mv_x[block]/2)
					#print("X: "+str(ix)+"Y: "+str(jy))
					if((0<=(ux)<xsp)and(0<=(uy)<ysp)):
						if(up_frame_valid[jy][ix] == 0):
							# handle blanks
							#up_frame[jy][ix] = frame[jy][ix]
							#up_frame[jy][ix] = [255,255,255]
							if(curr_mb == 'i' or curr_mb == 'I'):
								up_frame[jy][ix] = frame[jy][ix]
							else:
								up_frame[uy][ux] = next_frame[jy][ix] #try this?
						elif (up_frame_valid[jy][ix] > 1):
								# handle overlaps
								#up_frame[jy][ix] = [0,255,0]
								if(curr_mb == 'i' or curr_mb == 'I'):
									up_frame[jy][ix] = frame[jy][ix]
								else:
									up_frame[uy][ux] = next_frame[jy][ix]
	#print("Writing current frame")
	#input("Press Enter to continue...")
	#cv2.imshow('Frame',frame)
	#cv2.waitKey()
	#out.write(frame)
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

