import sys
import cv2
import re
import numpy as np
import os

mbfile = sys.argv[1]
os.rename(mbfile,mbfile+"~")

mb_source = open(mbfile+"~",'r')
mb_dest = open(mbfile,'w')

mb_line_start = re.compile("^\[h264 @.*\] ");
scene_cut_pattern = re.compile("scene cut .*\n");
mb_line = mb_source.readline();
print("Fuck 1")
while True:
	if not mb_line:
		break;
	else:
		#is_scene_cut = re.match(scene_cut_pattern,mb_line)
		if scene_cut_pattern.search(mb_line):
		#if is_scene_cut:
			print(mb_line)
			mb_line = re.sub(scene_cut_pattern,'',mb_line)
			print(mb_line)
			mb_line.rstrip()
			print(mb_line)
			mb_next = mb_source.readline()
			print(mb_next)
			mb_next = re.sub(mb_line_start,'',mb_next)
			print(mb_next)
			mb_line = mb_line+mb_next
			print(mb_line)
		mb_dest.write(mb_line)
		mb_line = mb_source.readline();

mb_source.close()
mb_dest.close()
os.remove(mbfile+"~")
