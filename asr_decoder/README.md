1. 將ASR和Decoder打包後直接搬
2. 將音檔的絕對路徑收集成txt
3. 執行decoder/run_decode.sh   

```
./run_decode.sh <input_list.txt>
```

4. 執行完後資料在stt_output.txt
5. 執行完後請將input_wave和input_feature移除