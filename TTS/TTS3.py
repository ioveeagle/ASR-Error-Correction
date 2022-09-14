#TTS version_3

import os
import random
from tkinter.tix import Select
from pydub import AudioSegment

# input輸入格是為: '檔名 context'
input_Path = 'TTS_output' 
output_Path = 'cut_audio1'

def cut(audio,milliseconds):
    #print(milliseconds-150,milliseconds+150)
    random_range = 50
    front = audio[:milliseconds-random_range]
    end = audio[milliseconds+random_range:]
    return front+end


if __name__ == '__main__':
    wav_file = os.listdir(input_Path)
    n = 1
    for filename in wav_file:
        print(n,'/',len(wav_file), '    ',filename)
        audio_path = f'{input_Path}/{filename}'
        audio = AudioSegment.from_wav(audio_path)
        #print(len(audio))

        # pick 3 times
        for i in range(3):

            # cutting len
            cut_point = [50,100,150]
            for point in cut_point:
                select = random.randint(1,len(audio)-1)
                # 防止超出範圍
                while (select < point | select+point > len(audio)):
                    select = random.randint(1,len(audio)-1)
                #print(select)
                limit = len(audio)-select
                output_audio = cut(audio,select,point)
                #print(len(output_audio))
                with open(f"{output_Path}{point*2}/{filename[:-4]}-{point*2}_{i+1}.wav", 'wb') as out_f:
                    output_audio.export(out_f, format='wav')
            print(i+1)
        print('-'*30)
        n +=1