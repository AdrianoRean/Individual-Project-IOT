# Use the sounddevice module
# http://python-sounddevice.readthedocs.io/en/0.3.10/

import numpy as np
import sounddevice as sd
import time

# Samples per second
sps = 44100

# Frequency / pitch
freq_hz = 6000

# Duration
duration_s = 15.0

# Attenuation so the sound is reasonable
atten = 1.0 # 0.3

# NumpPy magic to calculate the waveform
each_sample_number = np.arange(duration_s * sps)
waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / sps) + 3 * np.sin(2 * np.pi * 3* each_sample_number * freq_hz / sps)
waveform_quiet = waveform * atten

# Play the waveform out the speakers
#print(sd.query_devices())
sd.default.device = [2,2]

print("start")
sd.play(waveform_quiet, sps)
time.sleep(duration_s)
print("end")
sd.stop()
