import numpy as np

# Samples per second
sps = 1024

# Frequency / pitch
freq_hz = 100

# Duration
duration_s = 1.0

# Attenuation so the sound is reasonable
atten = 10.0 # 0.3

# NumpPy magic to calculate the waveform
each_sample_number = np.arange(duration_s * sps)
waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / sps) # + 3 * np.sin(2 * np.pi * 3* each_sample_number * freq_hz / 2 / sps)
waveform_quiet = waveform * atten

f = open("array.txt", "w")
for value in waveform_quiet:
    v = round(value,8)
    f.write(str(v) + ',')
f.close()

f = open("array2.txt", "w")
for value in waveform_quiet:
    v = round(value,8)
    f.write(str(v) + '\n')
f.close()

