#!/usr/bin/env python3
# -*- coding: cp1251 -*-
'''
test webfs_upload.py
Created on 06/03/2016.

@author: PVV
'''
#
#
#
import sys
#import argparse
import os
import subprocess

if __name__ == '__main__':
	if len(sys.argv) == 2:
		if sys.argv[1] == '-h':
			print 'Usage: labels.py eagle.app.v6.out labels.ld'
			sys.exit(0)
	elfname = "..\\app\\.output\\eagle\\image\\eagle.app.v6.out" 
	filename = "..\\ld\\labels.ld"

	if len(sys.argv) > 1: 
		if sys.argv[1]:
			elfname = sys.argv[1]
	if len(sys.argv) > 2:
		if sys.argv[2]:
			filename = sys.argv[2]
	symbols = {}
	try:
		ff = open(filename, "w")
	except:
		print "Error file open " + filename
		exit(1)
	try:
		tool_nm = "C:\\Espressif\\xtensa-lx106-elf\\bin\\xtensa-lx106-elf-nm.exe"
		if os.getenv("XTENSA_CORE") == "lx106":
			tool_nm = "xt-nm"
		proc = subprocess.Popen([tool_nm, "-n", elfname], stdout=subprocess.PIPE)
	except OSError:
		print "Error calling " + tool_nm + ", do you have Xtensa toolchain in PATH?"
		exit(1)
	for l in proc.stdout:
		fields = l.strip().split()
		try:
			symbols[fields[2]] = int(fields[0], 16)
#			print fields[2] + "\t" + fields[0]
			try:
				if fields[2].find("$") == -1 :
					if fields[2] != "call_user_start":
						ff.write("PROVIDE\t("+ fields[2] + " = 0x" + fields[0] + ");\n")
			except:
				print "Error write files: " + filename + " !"
				exit(1)				
		except ValueError as verr:
			pass  
		except Exception as ex:
			pass 
	ff.close()
	exit(0)