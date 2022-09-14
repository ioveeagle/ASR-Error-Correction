#TTS version_1

import os
import pyttsx3
import random
from py_chinese_pronounce import Word2pronounce,Pronounce2Word

# Para
# input輸入格是為: '檔名 context'
input_Path = 'text2wav.txt' 
output_Path = 'TTS_output'
change_wd = False
# 替換程度
finetune_range = [0.7,1]

# 判斷中文字
def is_chinese(strs):
    if not '\u4e00' <= strs <= '\u9fa5':
        return False
    else:
        return True

if __name__ == '__main__':
    # TTS model init
    engine = pyttsx3.init()
    # 音調控制
    voices = engine.getProperty('voices')
    engine.setProperty('voices', 'zh')
    # 語速控制
    rate = engine.getProperty('rate')
    engine.setProperty('rate', 150)  
    # 音量控制
    volume = engine.getProperty('volume')
    engine.setProperty('volume',1)

    # 找相似字套件
    p2w = Pronounce2Word()

    # jieba dict
    f = open('dict.txt',encoding="utf-8")
    dict = f.read()
    f.close

    print('start!')

    with open(input_Path,'r',encoding="utf-8") as file:
        data = file.readlines()
        for line in data:
            file_name, text = line.split( )
            o_text = text
            print(file_name,text)
            if (change_wd):
                # random select 要變換的字
                select_word = random.sample(range(0,len(text)-1),random.randint(round(len(text)*finetune_range[0]),round(len(text)-1*finetune_range[1])))
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
                        
                print(o_text, ' 轉換--> ',text)
            engine.save_to_file(text, f"{output_Path}/{file_name[:-4]}.wav")
            engine.runAndWait()
                
    print("Finish!")