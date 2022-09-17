#! /usr/bin/python
#coding=utf-8

#https://blog.csdn.net/u010764600/article/details/119704616

# Filename： wav2sbc.py

import sys
import os

def transfer(input):
	# 读sbc文件
	with open(input, "rb") as f:
		content = f.read()

	out = ""
	# 将其转化成Hex形式的TXT文件
	for i in range(len(content)):
		if i % 16 == 0 and i != 0:
			out += "\n";
		out += "0x%02x," % content[i]

	output = os.path.splitext(input)[0] + ".txt"
	# 输出到文件 test.txt
	with open(output, "w+") as f:
		f.write(out)

def wav2sbcHex(wavFilename):
	sbcFilename = os.path.splitext(wavFilename)[0] + ".opus"
	#cmd = "sox %s -r 16000 -c 1 -b 16 wavefiles/%s.wav" % (wavFilename, sbcFilename)
	#cmd = "ffmpeg -i %s -ar 16000 -ac 1 %s" % (wavFilename, sbcFilename)
	#print(cmd)
	#os.system(cmd)
	#if os.path.isfile(sbcFilename):
	#	print("Wav Transfer sbc Successfully");
	#else:
	#	print("Wav Transfer sbc Error")
	#	exit(1)

	transfer(sbcFilename)
	txtFilename = os.path.splitext(sbcFilename)[0] + ".txt"
	if os.path.isfile(txtFilename):
		print("Sbc Transfer Hex txt Successfully")
		print("Done")
	else:
		print("Sbc Transfer Hex txt Error")
		exit(1)

if __name__ == "__main__":
	for wavFilename in sys.argv[1:]:
		wav2sbcHex(wavFilename)
