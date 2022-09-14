使用方法
./computer-cer.pl <ref.file> <hyp.file> character

<ref.file>和<hyp.file>檔案每行的格式
文字\t句子ID

例如
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


#### Summary of 1 sentences
Sentence Accuracy: 0.00 (0/1)
Character-level:        Corr:66.67 Acc:66.67    C:6 M:4 D:0 I:0 S:2
