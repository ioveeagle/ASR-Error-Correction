# ASR-Error-Correction

產生ASR-Error-Correction 訓練資料

## TTS

### 對原文句調整後再轉成音訊檔

* [Version 1](./TTS1.py)

  `pip install pyttsx3, py_chinese_pronounce`

  使用 **Pyttsx3** 套件產生音檔 

  並使用 **py_chinese_pronounce** 替換 True Answer 中文字

  * 調整替換率

    ```python
    finetune_range = [0.7,1]  
    ```
* [Version 2](./TTS2.py)

  `pip install gTTS`

  使用 **gTTS** 套件產生音檔

  並使用 py_chinese_pronounce 替換 True Answer 中文字

  * 調整替換率

    ```python
    finetune_range = [0.7,1]  
    ```

### 將音訊檔轉為List做處理

* [Version 3](./TTS3.py)

  可利用上方兩隻py檔直接產生無調整之音檔(正確率極高)

  `from pydub import AudioSegment`

  再輸入至此py做處理

  隨機切除 50, 100, 150 ms 並重複做3次

## Usage

* 使用上述方法產生音訊檔
  
* 再將音訊檔丟入 asr_decoder 得到ASR的結果 
  1.  run kill_file.py
   
  2.  檔案位置輸入至test_input_list ([批次產生檔案位置](./create_path.py))
   
  3.  執行  ./run_decode.sh <input_list.txt>
   
  4.  執行完後資料在stt_output.txt
   
* 使用text_compare 比較原始文字與ASR結果 
  * 執行 ./computer-cer.pl <ref.file> <hyp.file> character
  
    ```
    <ref.file>
    今天天氣很好	001

    <hyp.file>
    今天天色很差	001

    > ./computer-cer.pl ref.file hyp.file character
    FILE: 001
    REF: 今 天 天 氣 很 好
    HYP: 今 天 天 色 很 差
    Edit_Distance: 2
    Utterance:      Corr:66.67 Acc:66.67    C:6 M:4 D:0 I:0 S:2
    MCH: 今 -> 今
    MCH: 天 -> 天
    MCH: 天 -> 天
    SUB: 氣 -> 色
    MCH: 很 -> 很
    SUB: 好 -> 差
    ```
  
* [將原始文字與ASR結果 匯出至TSV比較](./toTSV.py)


## Compare

| |gTTS|Pyttsx3|
|-----|--------|--|
|Speed| 快  | 較慢 |
| 可以調整項目 | 無  | 音調、語速、音量 |