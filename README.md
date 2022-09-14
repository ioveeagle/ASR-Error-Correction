# ASR-Error-Correction

產生ASR-Error-Correction 訓練資料

## TTS

### 對原文句調整後再轉成音訊檔

* [Version 1](./TTS1.py)

  `pip install pyttsx3, py_chinese_pronounce`

  使用 **Pyttsx3** 套件產生音檔 [可以調整音調、語速、音量]

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
