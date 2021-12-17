#!/usr/bin/env python
#coding: utf-8

import socket	
from time import sleep


host = 'localhost'
port = 20001

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((host, port))

client.send('xxxxxx')

sleep (1)

client.send('aaa\r\nbbb\r\nccc')

sleep (1)

client.send('dddddd\r\n')

sleep (1)

client.send('\r\n')
client.send('\r\n')
client.send('\r\n')

client.send('ee\r\n')


