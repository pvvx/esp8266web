#-*- coding: utf-8 -*-
#
# test webfs_upload.py
# PV` Created on 26/09/2015.
#
# Bases:
#---------------------------------------------------
# 2006/02 Will Holcomb <wholcomb@gmail.com>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# 2007/07/26 Slightly modified by Brian Schneider  
#
# in order to support unicode files ( multipart_encode function )
# From http://peerit.blogspot.com/2007/07/multipartposthandler-doesnt-work-for.html
#
# 2013/07 Ken Olum <kdo@cosmos.phy.tufts.edu>
#
# Removed one of \r\n and send Content-Length
#
# 2014/05 Applied Fedora rpm patch
#
# https://bugzilla.redhat.com/show_bug.cgi?id=920778
# http://pkgs.fedoraproject.org/cgit/python-MultipartPostHandler2.git/diff/python-MultipartPostHandler2-cut-out-main.patch?id=c1638bb3e45596232b4d02f1e69901db0c28cfdb
#
# 2014/05/09 Sergio Basto <sergio@serjux.com>
#
# Better deal with None values, don't throw an exception and just send an empty string.
#
#---------------------------------------------------
import sys
import urllib
import urllib2
import mimetools #, mimetypes
import os
import stat
from base64 import standard_b64encode
	   
from cStringIO import StringIO

class Callable:
	def __init__(self, anycallable):
		self.__call__ = anycallable

class MultipartPostHandler(urllib2.BaseHandler):
	handler_order = urllib2.HTTPHandler.handler_order - 10 # needs to run first
	

	def http_request(self, request):
		data = request.get_data()
		if data is not None and type(data) != str:
			v_files = []
			v_vars = []
			try:
				for(key, value) in data.items():
					if type(value) == file:
						v_files.append((key, value))
					else:
						v_vars.append((key, value))
			except TypeError:
				systype, value, traceback = sys.exc_info()
				raise TypeError, "not a valid non-string sequence or mapping object", traceback
			if len(v_files) == 0:
				data = urllib.urlencode(v_vars, 1)
			else:
				boundary, data = self.multipart_encode(v_vars, v_files)
				contenttype = 'multipart/form-data; boundary=%s' % boundary
								
				if(request.has_header('Content-Type')
				   and request.get_header('Content-Type').find('multipart/form-data') != 0):
					print "Replacing %s with %s" % (request.get_header('content-type'), 'multipart/form-data')

				request.add_unredirected_header('Content-Type', contenttype)
#				authstr = 'Basic ' + standard_b64encode('ESP8266' + ':' + '0123456789')
#				if(request.has_header('Authorization')):
#					print "Replacing %s with %s" % (request.get_header('Authorization'), authstr)
#				request.add_unredirected_header('Authorization', authstr)
				request.add_data(data)
		return request

	def multipart_encode(vars, files, boundary = None, buffer = None):
		if boundary is None:
			boundary = mimetools.choose_boundary()
		if buffer is None:
			buffer = StringIO()
		for(key, value) in vars:
			buffer.write('--%s\r\n' % boundary)
			buffer.write('Content-Disposition: form-data; name="%s"' % key)
			if value is None:
				value = ""
			# if type(value) is not str, we need str(value) to not error with cannot concatenate 'str'
			# and 'dict' or 'tuple' or somethingelse objects
			buffer.write('\r\n\r\n' + str(value) + '\r\n')
		for(key, fd) in files:
			file_size = os.fstat(fd.fileno())[stat.ST_SIZE]
			filename = fd.name.split('/')[-1]
#			contenttype = mimetypes.guess_type(filename)[0] or 'application/octet-stream'
			contenttype = 'application/octet-stream'
			buffer.write('--%s\r\n' % boundary)
			buffer.write('Content-Disposition: form-data; name="%s"; filename="%s"\r\n' % (key, filename))
			buffer.write('Content-Type: %s\r\n' % contenttype)
			buffer.write('Content-Length: %s\r\n' % file_size)
			fd.seek(0)
			buffer.write('\r\n' + fd.read() + '\r\n')
		buffer.write('--' + boundary + '--\r\n')
		buffer = buffer.getvalue()
		return boundary, buffer
	multipart_encode = Callable(multipart_encode)

	https_request = http_request
		
if __name__ == '__main__':
	if len(sys.argv) == 2:
		if sys.argv[1] == '-h':
			print 'Usage: filename espurl username password'
			sys.exit(0)

	filename = '../webbin/WEBFiles.bin'
	espurl = 'http://sesp8266/fsupload'
	username = 'ESP8266'
	password = '0123456789'

	if len(sys.argv) > 1: 
		if sys.argv[1]:
			filename = sys.argv[1]
	if len(sys.argv) > 2: 
		if sys.argv[2]:
			espurl = sys.argv[2]
	if len(sys.argv) > 3:
		if sys.argv[3]:
			username = sys.argv[3]
	if len(sys.argv) > 4:
		if sys.argv[4]:
			password = sys.argv[4]
	
	print('Start send %s to %s' % (filename, espurl))
	opener = urllib2.build_opener(MultipartPostHandler)
	authstr = 'Basic ' + standard_b64encode(username + ':' + password)
	opener.addheaders.append(['Authorization', authstr])
	params = { 'overlay' : open(filename, 'rb') }
	try:
		resp = opener.open(espurl, params)
		print('End, response code: %s\n' % resp.code)
		sys.exit(0)    
	except Exception as e:
		print('Failed open (%s) %s\n' % (str(e).decode('cp1251'), espurl))
		sys.exit(1)    

	