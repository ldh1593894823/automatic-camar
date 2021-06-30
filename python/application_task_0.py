import ctypes
import serial
import multiprocessing
import time
import cv2
import yaml

from sys import exit
from ctypes import *

CFG = yaml.safe_load(open("config.yaml"))

def send_data(now_, spt_,led_):               
    ser = serial.Serial(CFG["SerialPort"]["Port"], 9600, 5)

    class position(Structure):
        _fields_ = [('now_x', c_uint16),
                    ('now_y', c_uint16),
                    ('p', c_float),
                    ('led',c_uint8)]
    x = position()
    x.now_x = now_
    x.now_y = spt_
    x.p = 0.13
    x.led = led_
    # print(string_at(addressof(x), sizeof(x)).hex(), now_, spt_)
    ser.write(ctypes.string_at(ctypes.addressof(x), ctypes.sizeof(x)))

# reda_data(int(ser.read(1)))

def pictures():
    image = cv2.imread('2021--06--10 19.25.44.png')  ## 图片
    image = cv2.copyMakeBorder(image, 0, 6, 0, 6,cv2.BORDER_CONSTANT,value=(255, 255, 255))
    image = cv2.hconcat([image,image,image])
    image = cv2.vconcat([image,image,image])
    image = cv2.copyMakeBorder(image, 6, 0, 6, 0,cv2.BORDER_CONSTANT,value=(255, 255, 255))
    cv2.imshow('image',image)
    cv2.imwrite('1.png', image)
    cv2.waitKey(None)

def Task_0(Q: multiprocessing.Queue):
    cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
    while (1):
        x, y, w, h = 0, 0, 0, 0
        ret, frame = cap.read()
        face_cascade = cv2.CascadeClassifier()
        face_cascade.load('haarcascade_frontalface_default.xml')

        faces = face_cascade.detectMultiScale(frame, scaleFactor=1.3, minNeighbors=5)

        img1 = frame.copy()
        img2 = frame.copy()
        for x, y, w, h in faces:
            img2 = cv2.rectangle(img1, (x, y), (x + w, y + h), (0, 255, 0), 2)

        if len(faces):
            x = faces[0][0]
            y = faces[0][1]
            w = int(faces[0][2]/2)
            h = int(faces[0][3]/2)
            cv2.circle(img2, (x+w, y+h), 9, (0, 0, 255), -1, 4)
            send_data(x+w, y+h,0)
            image = cv2.putText(img2, str(x+w), (x+w, y+h), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
        else:
            image = img2

        cv2.imshow('img', image)
        cv2.waitKey(1)
        print("图片亮度值为：", cv2.cvtColor(image, cv2.COLOR_BGR2GRAY).mean())


        if not Q.empty():
            k = Q.get()
            if k == 'q':
                cv2.destroyAllWindows()
                exit(0)
            elif k == 's':
                send_data(x+w, y+h,1)
                now = int(time.time())
                timeArray = time.localtime(now)
                otherStyleTime = time.strftime("%Y--%m--%d %H.%M.%S", timeArray)
                print(otherStyleTime)
                cv2.imwrite(str(otherStyleTime) + '.png', frame[80:480, 170:470])