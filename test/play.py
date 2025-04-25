import serial
import wave
import time
import numpy as np

SERIAL_PORT = 'COM14'
BAUD_RATE = 115200
SAMPLE_RATE = 8000
DURATION = 5
OUTPUT_FILE = 'output.wav'

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT}")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit(1)

audio_data = []
samples_received = 0
start_time = time.time()

print("Recording...")

while time.time() - start_time < DURATION:
    try:
        if ser.in_waiting > 0:
            # Read all available bytes
            data = ser.read(ser.in_waiting)
            for sample in data:
                # Convert to signed 8-bit integer (-128 to 127)
                value = int.from_bytes([sample], 'little', signed=True)
                audio_data.append(value)
                samples_received += 1
    except serial.SerialException as e:
        print(f"Error reading from serial port: {e}")
        break

ser.close()

print(f"Total samples collected: {samples_received}")
print(f"Expected samples: {SAMPLE_RATE * DURATION}")

if samples_received == 0:
    print("No data received. Check your connections and ESP8266 code.")
    exit(1)

# Convert to numpy array for processing
audio_array = np.array(audio_data, dtype=np.int8)

# Normalize audio if needed
if np.max(np.abs(audio_array)) > 0:
    audio_array = (audio_array / np.max(np.abs(audio_array)) * 127).astype(np.int8)

# Save to WAV file
with wave.open(OUTPUT_FILE, 'wb') as wav_file:
    wav_file.setnchannels(1)
    wav_file.setsampwidth(1)  # 8-bit audio
    wav_file.setframerate(SAMPLE_RATE)
    wav_file.writeframes(audio_array.tobytes())

print(f"Saved audio to {OUTPUT_FILE}")