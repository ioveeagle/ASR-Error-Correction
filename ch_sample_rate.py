#改變取樣率

import os
import subprocess

input_path = r"C:\Users\KYU_NLPLAB\Desktop\code\Delta\TTS2_data"
output_path = r"C:\Users\KYU_NLPLAB\Desktop\code\Delta\TTS2_data"

for file in os.listdir(input_path):
    file1 = input_path+'\\'+file
    file2 = output_path+'\\'+file
    cmd = "ffmpeg -i " + file1 + " -ar 8000 " + file2  #ffmpeg -i 输入文件 -ar 采样率  输出文件
    subprocess.call(cmd, shell=True)