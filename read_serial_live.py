import serial
import time
import sys

print("Opening COM6...")
s = serial.Serial('COM6', 115200, timeout=1)
time.sleep(0.1)

# Clean input buffer
s.reset_input_buffer()

print("Listening for 30 seconds... Please swipe the card now!")
start_time = time.time()
try:
    while time.time() - start_time < 30:
        if s.in_waiting > 0:
            line = s.readline().decode('utf-8', 'ignore')
            if line:
                print(line, end='')
                sys.stdout.flush()
        time.sleep(0.01)
except KeyboardInterrupt:
    pass

s.close()
print("\nClosed port.")
