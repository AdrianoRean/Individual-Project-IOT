import numpy as np

# Samples per second
sps = 1000

# Frequency / pitch
freq_hz = 3

# Duration
duration_s = 1.0

# Attenuation so the sound is reasonable
atten = 100.0 # 0.3

# NumpPy magic to calculate the waveform
each_sample_number = np.arange(duration_s * sps)
waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / sps) # + 3 * np.sin(2 * np.pi * 3* each_sample_number * freq_hz / 2 / sps)
waveform_quiet = waveform * atten

f = open("array.txt", "w")
for value in waveform_quiet:
    f.write(str(value) + ',')
f.close()

