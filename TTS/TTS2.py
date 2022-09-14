#TTS version_2

from gtts import gTTS
import os
import librosa
import random
from py_chinese_pronounce import Word2pronounce,Pronounce2Word

# Para
# input輸入格是為: '檔名 context'
input_Path = 'wav2sent.txt' 
output_Path = 'TTS1_data'
change_wd = False
# 替換程度
finetune_range = [0,0.3]

# 判斷中文字
def is_chinese(strs):
    if not '\u4e00' <= strs <= '\u9fa5':
        return False
    else:
        return True

# gTTS speak
def tts_speak(file_name, text):
    tts = gTTS(text=text, lang='zh-TW')
    tts.save(f"TTS2_data/{file_name[:-4]}.wav")

if __name__ == '__main__':
    # 找相似字套件
    p2w = Pronounce2Word()

    # jieba dict
    f = open('dict.txt',encoding="utf-8")
    dict = f.read()
    f.close

    print('start!')

    with open('wav2sent.txt','r',encoding="utf-8") as file:
        data = file.readlines()
        
        for line in data:
            file_name, text = line.split( )
            print(file_name,text)
            if (change_wd):
                # random select 要變換的字
                select_word = random.sample(range(0,len(text)-1),random.randint(round(len(text)*finetune_range[0]),round(len(text)*finetune_range[1])))
                t_list = list(text)
                for i in select_word:
                    # 中文
                    if is_chinese(t_list[i]):
                        # 回傳相似字
                        similar_list = p2w.find_similar(t_list[i])
                        similar_list_filter = []
                        # 只取常見字
                        for k in similar_list:
                            if k in dict: similar_list_filter.append(k)
                        
                        if(len(similar_list_filter)) != 0: 
                            choice = random.randint(0,len(similar_list_filter)-1)
                            t_list[i] = similar_list_filter[choice]
                            text = ''.join(t_list)
                print('轉換--> ',text)
            

                tts_speak(file_name,text)
    print("Finish!")