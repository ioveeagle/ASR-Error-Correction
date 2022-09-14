# sox 未完成

import librosa
import os
import IPython.display as ipd
import matplotlib.pyplot as plt
import librosa.display


wav_file = os.listdir("8k")

for filename in wav_file[:3]:
    audio_path = f'8k/{filename}'
    x , sr = librosa.load(audio_path)
    print(type(x), type(sr))
    print(x.shape, sr)
    librosa.load(audio_path, sr=44100)
    plt.figure(figsize=(14, 5))
    librosa.display.waveplot(x, sr=sr)