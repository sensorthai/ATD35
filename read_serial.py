import serial
import time

s = serial.Serial('COM6', 115200, timeout=1)
time.sleep(0.1)

# Reset the board by toggling DTR/RTS
s.dtr = False
s.rts = True
time.sleep(0.1)
s.rts = False
time.sleep(1.5)

# Read boot log
for _ in range(100):
    line = s.readline().decode('utf-8', 'ignore')
    if line:
        print(line, end='')

s.close()
