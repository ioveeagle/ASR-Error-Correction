#!/bin/bash
### USAGE: ./run_decode.sh <input_list>
### OUTPUT: stt_output.txt
### NOTE: you can remove ./input_wave and ./input_feature each usage

set -e

dir_model="seedmodel"
dir_wave="input_wave"
dir_feature="input_feature"
infile=$1

### 檢查執行檔是否存在
for cmd in $(which sox) ./featureExtraction/feature.x ./recog-dnn.sse2; do
	if [[ ! -x $cmd ]]; then echo -e "\e[31mERROR: $cmd is not an executable!\e[0m" && exit 1; fi
done


### 輸入音檔列表檢查
if [[ ! -e $infile ]]; then
	echo -e "\e[31mERROR: Cannot locate input list file $infile!\e[0m"
	exit 1
fi



mkdir -p $dir_feature
mkdir -p $dir_wave

### 音檔取樣率處理
awk -v DIR_WAVE="$PWD/$dir_wave" '{  file = $1; gsub(/^.*\//, "", file); printf("sox %s -e signed-integer -b 16 -c 1 -r 8000 %s/%s\n", $1, DIR_WAVE, file)}' $infile > run_wave.sh
bash ./run_wave.sh

### 特徵參數萃取
ls -1 $dir_wave/*.wav > list_wav
./featureExtraction/feature.x list_wav $PWD $PWD/$dir_feature MVNMA

### 語音辨識
ls -1 $PWD/$dir_feature/$dir_wave/*mvacjp_atc | awk '{printf "TEST\t(spk_%s)\n", $1}' > list_feat
./recog-dnn.sse2 $dir_model/DEST.graph $dir_model/DNN.macro list_feat $dir_model/DEST.table $dir_model/DEST.sym $dir_model/DEST.dic 3000 13 > stt_output.txt
