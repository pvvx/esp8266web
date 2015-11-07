#!/usr/bin/env python3
# -*- coding: cp1251 -*-
'''
test webfs_upload.py
Created on 27/09/2015.

@author: PVV
'''
#
# Use module 'requests' - http://docs.python-requests.org/
#
 
import requests 
from requests.auth import HTTPBasicAuth
 
 
if __name__ == '__main__':
    espurl = 'http://192.168.4.1/fsupload'
    username = 'ESP8266'
    password = '0123456789'
    filename = '../webbin/WEBFiles.bin'

    files = {'file': (filename, open(filename, 'rb'), 'application/octet-stream')}
#    postdata = {'':''}
#    headers = {'Connection' : 'keep-alive'}
    
    s = requests.Session()
#    q = s.post(theurl, auth=HTTPBasicAuth(username, password), headers=headers, data=postdata, files=files)
    q = s.post(espurl, auth=HTTPBasicAuth(username, password), files=files)
    print(q.status_code)
