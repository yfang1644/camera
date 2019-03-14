#!/usr/bin/python
# -*- encoding: utf-8  -*-

import socket
import time
import hashlib
import cv2

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(("", 6964))    #地址和端口号
server.listen(5) #监听

print("监听开始..")

while True:
    conn, addr = server.accept()

    print("conn:", conn, "\naddr:", addr)

    while True:
        data = conn.recv(1024)
        if not data:
            print("客户端断开连接")
            conn.close()
            break

        cmd = data.decode()
        if cmd == "shoot":
            cap = cv2.VideoCapture(0)
            frame = cap.read()[1]
            cap.release()
            frame_enc = frame[:,:,0].tostring()
            #发送时间
            t_str = time.strftime("%Y-%m-%d-%H:%M:%S",time.localtime())
            conn.send(t_str.encode())
            conn.recv(1024)
            #发送大小
            size = len(frame_enc)
            conn.send(str(size).encode())
            conn.recv(1024)
            #发送长宽
            framesize = "{},{}".format(frame.shape[0], frame.shape[1])
            conn.send(framesize.encode())
            conn.recv(1024)

            #发送文件内容
            m = hashlib.md5()
            conn.send(frame_enc)
            m.update(frame_enc)
            conn.recv(1024)

            #md5校验
            md5 = m.hexdigest()
            conn.send(md5.encode())

        if cmd == "exit":
            server.close()
            exit()
