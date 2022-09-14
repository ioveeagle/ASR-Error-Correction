# create 丟入ASR路徑
so = open('text2wav.txt',encoding="utf-8")
path_txt = open("path.txt",'w', encoding='utf8')
for line in so.readlines():
    file_name, text = line.split( )
    print(line)
    path_txt.write(f"/user_data/code/Delta/TTS_ASR/cut_audio1/{file_name}\n")