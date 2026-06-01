import serial
import time

s = serial.Serial('COM6', 115200, timeout=1)
start = time.time()
print("=== Serial Monitor (60 sec) - Swipe card now! ===")
while time.time() - start < 60:
    line = s.readline()
    if line:
        text = line.decode('utf-8', 'replace').strip()
        if text:
            print(text)
s.close()
print("=== Monitor ended ===")
