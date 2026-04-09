import serial
import time

s = serial.Serial('COM11', 115200, timeout=0.5)
print("等待串口数据...")
time.sleep(0.5)

start = time.time()
while time.time() - start < 8:
    line = s.readline().decode('utf-8', 'ignore').strip()
    if line:
        print(line)

s.close()
