# ASR結果整理成TSV

from pyparsing import And
import csv

from requests import delete

table = [['no','file name','file number','answer','result_1(0%)','result_2(random)','replace','insert','delete']]

print('匯入ans')
compare1 = open('text2wav.txt',encoding="utf-8")
output_ans = open("output_ans.txt",'w', encoding='utf8')
n = 1
for line in compare1.readlines():
    file_name, text = line.split( )
    cut1 = file_name.find('_')
    cut2 = file_name.find('.wav')
    table.append([n,file_name[:15],file_name[cut1+1:cut2],text,'','','','',''])
    output_ans.writelines([f'{text.replace(" ","")}\t',f'{n}\n'])
    n+=1
compare1.close
output_ans.close

print('匯入result1')
compare4 = open('compare4.txt',encoding="utf-8")
output_txt4 = open("output_txt4.txt",'w', encoding='utf8')

for line in compare4.readlines():
    cut1 = line.find('(spk')
    text2 = line[:cut1-1]
    cut2 = line.find('82119')
    cut3 = line.find('.wav')
    file_name = line[cut2:cut2+15]
    num = line[cut2+16:cut3]
    for i in table:
        if (i[1]==file_name) & (i[2]==num): 
            i[4]=text2.replace(" ","")
            n = i[0]
    output_txt4.writelines([f'{text2.replace(" ","")}\t',f'{n}\n'])
compare4.close
output_txt4.close


print('匯入result2')
compare5 = open('compare5.txt',encoding="utf-8")
output_txt5 = open("output_txt5.txt",'w', encoding='utf8')

for line in compare5.readlines():
    cut1 = line.find('(spk')
    text3 = line[:cut1-1]
    cut2 = line.find('82119')
    cut3 = line.find('.wav')
    file_name = line[cut2:cut2+15]
    num = line[cut2+16:cut3]
    for i in table:
        if (i[1]==file_name) & (i[2]==num): 
          i[5]=text3.replace(" ","")
          n = i[0]
    output_txt5.writelines([f'{text3.replace(" ","")}\t',f'{n}\n'])
compare5.close
output_txt5.close

print('匯出至tsv')
with open(r'compare.tsv', 'w', newline='',encoding="utf-8") as f:
    writer = csv.writer(f, delimiter='\t')
    # 寫入二維表格
    writer.writerows(table)

print('finish')