#!/usr/bin/python3
# -*- encoding: utf-8 -*-

import socket
import os
import hashlib
import cv2
import numpy
from time import sleep

client = socket.socket()

ip_port =("192.168.2.106", 6964)  #地址和端口号

client.connect(ip_port)  #连接

print("服务器已连接")

if not os.path.exists("photo"):
    os.mkdir("photo")
while True:
    
    #手动挡,输入shoot拍摄
    #cmd = input(">>")
    #interval = 0
    
    #自动档
    cmd = "shoot"
    interval = 5

    if len(cmd)==0: continue
    else:
        client.send(cmd.encode())
        #接受拍摄时间
        img_time = client.recv(1024).decode()
        client.send("time got".encode())

        #接收长度
        server_response = client.recv(1024)
        im_size = int(server_response.decode())
        print("接收到的大小：", im_size)
        client.send("size got".encode())

        #接收长宽
        framesize = client.recv(1024).decode().split(',')
        frame_height = int(framesize[0])
        frame_width = int(framesize[1])
        client.send("framesize got".encode())

        #接收文件内容
        client.send("ready".encode())  #接收确认
        
        received_size = 0
        m = hashlib.md5()
        frame_bytes=b""

        while received_size < im_size:
            size = 0  #准确接收数据大小，解决粘包
            if im_size - received_size > 1024: #多次接收
                size = 1024
            else:  #最后一次接收
                size = im_size - received_size

            data = client.recv(size)  # 多次接收内容，接收大数据
            frame_bytes += data
            data_len = len(data)
            received_size += data_len

        m.update(frame_bytes)
        
        client.send("check".encode())
        #md5值校验
        md5_server = client.recv(1024).decode()
        md5_client = m.hexdigest()
        if md5_server == md5_client:
            print("MD5值校验成功")
        else:
            print("MD5值校验失败")
        #解码
        frame=numpy.frombuffer(frame_bytes,dtype=numpy.uint8).reshape(frame_height,frame_width)
        cv2.imwrite("photo/%s.png"%img_time,frame)

        sleep(interval)

client.close()
