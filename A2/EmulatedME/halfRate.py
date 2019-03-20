import sys
import cv2
import numpy as np

infile = sys.argv[1];
outfile = sys.argv[2];
print ("Input: "+infile+" Output"+outfile)

# Create a VideoCapture object and read from input file
# If the input is the camera, pass 0 instead of the video file name
cap = cv2.VideoCapture(infile)
fourcc = cv2.VideoWriter_fourcc('m','p','4','v')
out = cv2.VideoWriter()
 
if (cap.isOpened()== False): 
  print("Error opening input video stream or file")
  sys.exit(1)

width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
fps = int(cap.get(cv2.CAP_PROP_FPS))
frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
print ("W = "+str(width)+" H = "+str(height) + " FPS = "+str(fps) + " Frames = "+str(frames))

o_work = out.open(outfile,fourcc,fps/2,(width,height),True)
num_frames = frames

for i in range (0,frames):
  ret,frame = cap.read()
  if (i%2==0):
    out.write(frame)

# Read until video is completed
#while(cap.isOpened()):
  # Capture frame-by-frame
#  ret, frame = cap.read()
#  if ret == True:

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