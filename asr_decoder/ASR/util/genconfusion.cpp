#include "recog.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>

using namespace std;

#define MAX_WORD_NUM 10
#define MAX_CONFUSION_NUM 20

typedef struct{
	char sylStr[20];
	int *occurrence;
	float *ratio;
} CONFUSIONMATRIX;

typedef struct{
	char word[20];
	int num;
	int confusion[MAX_CONFUSION_NUM];
} CONTEXTWORD;

typedef struct{
	char src[20];
	char dst[30];
} DICMAP;

class GENCONFUSION{
private:
	int matrixDim;
	CONFUSIONMATRIX *cm;
	CONTEXTWORD *cw;
	DICMAP *c2s, *s2ci;
	int c2sNum, s2ciNum;
	void readConfusionMatrix(char *matrixFile);
	void getSingleWord(char *str);
	int wordNum;	
	void obtainConfusion();
	void readDic(char *fileName, DICMAP *dm, int dmNum);
	void sortDic(DICMAP *list, int low, int up);
	int binarySearch(DICMAP *list, int left, int right, char *inStr);
	void outputContext(FILE *f);
	UTILITY U;
public:
	GENCONFUSION();
	~GENCONFUSION();
	void geConfusionEntry(char *ctxIn, char *char2syl, char *syl2ci, char *cmFile);
};

GENCONFUSION::GENCONFUSION(){
	cm=NULL;
	cw=new CONTEXTWORD[MAX_WORD_NUM];
	c2s=NULL;
	s2ci=NULL;
}

GENCONFUSION::~GENCONFUSION(){
	if(cm!=NULL){
		for(int i=0;i<matrixDim;i++){
			if(cm[i].occurrence!=NULL){
				delete [] cm[i].occurrence;
				delete [] cm[i].ratio;
				cm[i].occurrence=NULL;
				cm[i].ratio=NULL;
			}
		}
		delete [] cm;
		cm=NULL;
	}
	if(cw!=NULL){
		delete [] cw;
		cw=NULL;
	}
	if(c2s!=NULL){
		delete [] c2s;
		c2s=NULL;
	}
	if(s2ci!=NULL){
		delete [] s2ci;
		s2ci=NULL;
	}
}

void GENCONFUSION::getSingleWord(char *str){
	char msg[MAXLEN], *pch;
	strcpy(msg, str);

	wordNum=0;
	pch=strtok(msg, " ");
	while(pch!=NULL){
		strcpy(cw[wordNum].word, pch);
		wordNum++;
		pch=strtok(NULL, " ");
		if(wordNum>=MAX_WORD_NUM){
			printf("keyword length exceeds %d\n", MAX_WORD_NUM);
			exit(1);
		}
	}
}

void GENCONFUSION::readConfusionMatrix(char *matrixFile){
	int cnt;

	matrixDim=U.getFileLineCount(matrixFile);
	cm=new CONFUSIONMATRIX[matrixDim];

	char msg[MAXLEN*3], *pch;
	FILE *in=U.openFile(matrixFile, "rt");
	// i-th line denotes i-th word
	for(int i=0;i<matrixDim;i++){
		cnt=0;
		cm[i].occurrence=new int[matrixDim];
		cm[i].ratio=new float[matrixDim];

		fgets(msg, MAXLEN*3, in);
		msg[strlen(msg)-1]='\0';
		pch=strtok(msg, "\t ");
		strcpy(cm[i].sylStr, pch);
		pch=strtok(NULL, "\t ");
		while(pch!=NULL){
			cm[i].occurrence[cnt]=atoi(pch);
			cnt++;
			pch=strtok(NULL, "\t ");
		}
	}
	fclose(in);

	int sum;
	for(int i=0;i<matrixDim;i++){
		sum=0;
		for(int j=0;j<matrixDim;j++){
			sum+=cm[i].occurrence[j];
			if(cm[i].occurrence[j]<0)
				int de=0;
		}
		// the probability of i-th word is recognized as j-th word
		for(int j=0;j<matrixDim;j++)
			cm[i].ratio[j]=(float)cm[i].occurrence[j]/sum;
	}
}

void GENCONFUSION::sortDic(DICMAP *list, int low, int up){
	int i=low, j=up;
	char key[MAXLEN], temp[MAXLEN];
	strcpy(key, list[(low+up)/2].src);
	//  partition
	do{
		while (strcmp(list[i].src,key)<0) i++;
		while (strcmp(list[j].src,key)>0) j--;
		if (i<=j){
			strcpy(temp, list[i].src); strcpy(list[i].src, list[j].src); strcpy(list[j].src, temp);
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (low<j) sortDic(list, low, j);
	if (i<up) sortDic(list, i, up);
}

int GENCONFUSION::binarySearch(DICMAP *list, int left, int right, char *inStr){
	int mid=(left+right)/2,difference;
	difference=strcmp(inStr,list[mid].src);
	if (difference==0)
		return mid;
	(difference>0)?left=mid+1:right=mid-1;
	return ((left>right)||(right<left))?-1:binarySearch(list,left,right,inStr);
}

void GENCONFUSION::readDic(char *fileName, DICMAP *dm, int dmNum){
	char msg[MAXLEN], *p1, *p2;
	FILE *in=U.openFile(fileName, "rt");
	for(int i=0;i<dmNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		p1=strtok(msg, "\t");
		p2=strtok(NULL, "\t");

		strcpy(dm[i].src, p1);
		strcpy(dm[i].dst, p2);
	}
	fclose(in);

	sortDic(dm, 0, dmNum-1);
}

void GENCONFUSION::obtainConfusion(){
	int found, cmPos;
	// from word to syl
	for(int i=0;i<wordNum;i++){
		// syl position
		found=binarySearch(c2s, 0, c2sNum-1, cw[i].word);
		if(found==-1){
			printf("%s not found in dic\n", cw[i].word);
			exit(1);
		}
		// syl str -> cm position
		cmPos=-1;
		for(int j=0;j<matrixDim;j++){
			if(strcmp(cm[j].sylStr, c2s[found].dst)==0){
				cmPos=j;
				break;
			}
		}
		if(cmPos==-1){
			printf("%s not found in confusion matrix\n", c2s[found].dst);
			exit(1);
		}
		// pick up ratio>0.1
		cw[i].num=0;
		for(int j=0;j<matrixDim;j++){
			if(cm[cmPos].ratio[j]>0.1){
				cw[i].confusion[cw[i].num]=j;
				cw[i].num++;
				if(cw[i].num>=MAX_CONFUSION_NUM){
					printf("Confusion num exceeds %d\n", MAX_CONFUSION_NUM);
				}
			}
		}
	}
}

void GENCONFUSION::outputContext(FILE *f){
	char outStr[MAXLEN][MAX_WORD_NUM][20];
	int row, num, repeatNum[MAX_WORD_NUM], r;

	row=1;
	for(int i=0;i<wordNum;i++)
		row*=cw[i].num; // how many lines needed for i-th syllable
	if(row>MAXLEN){
		printf("Number of combinations exceeds %d\n", MAXLEN);
		exit(1);
	}
	num=row;
	// get combinations
	for(int i=0;i<wordNum;i++)
		repeatNum[i]=1;
	for(int i=0;i<wordNum;i++){
		repeatNum[i]=num/cw[i].num; // a ratio>1
		num=num/cw[i].num;
	}
	for(int i=0;i<wordNum;i++){
		r=0;
		while(r<row){
			for(int j=0;j<cw[i].num;j++){// loopNum of a word
				for(int k=0;k<repeatNum[i];k++){// loopNum of a variant
					strcpy(outStr[r][i], cw[i].word);
					strcat(outStr[r][i], "_");
					strcat(outStr[r][i], cm[cw[i].confusion[j]].sylStr);
					r++;
				}
			}
		}
	}
	// output to file
	char msg[MAXLEN];
	for(int i=0;i<row;i++){
		strcpy(msg, "");
		for(int j=0;j<wordNum;j++){
			strcat(msg, outStr[i][j]);
			if(j+1==wordNum)
				strcat(msg, "_@"); // indicate the end
			strcat(msg, " ");
		}
		msg[strlen(msg)-1]='\0';
		fprintf(f, "%s\n", msg);
	}
}

void GENCONFUSION::geConfusionEntry(char *ctxIn, char *char2syl, char *syl2ci, char *cmFile){
	FILE *in, *f;
	char msg[MAXLEN], *pch;
	int lineNum;

	readConfusionMatrix(cmFile);

	c2sNum=U.getFileLineCount(char2syl);
	c2s=new DICMAP[c2sNum];
	readDic(char2syl, c2s, c2sNum);

	s2ciNum=U.getFileLineCount(syl2ci);
	s2ci=new DICMAP[s2ciNum];
	readDic(syl2ci, s2ci, s2ciNum);

	// LM.confusion
	lineNum=U.getFileLineCount(ctxIn);
	in=U.openFile(ctxIn, "rt");
	f=U.openFile("LM.confusion", "wt");
	for(int i=0;i<lineNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		getSingleWord(msg);
		obtainConfusion();
		outputContext(f);
	}
	fclose(in);
	fclose(f);
	// dic.confusion
	char cfFile[MAXLEN];
	strcpy(cfFile, "LM.confusion");

	vector<string> vect;
	lineNum=U.getFileLineCount(cfFile);
	in=U.openFile(cfFile, "rt");
	for(int i=0;i<lineNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		pch=strtok(msg, " ");
		while(pch!=NULL){
			vect.push_back(pch);
			pch=strtok(NULL, " ");
		}
	}
	fclose(in);

	sort(vect.begin(), vect.end());
	vector<string>::iterator it=unique(vect.begin(), vect.end());
	vect.erase(it, vect.end());

	strcpy(cfFile, "DIC.confusion");
	f=U.openFile(cfFile, "wt");
	int entryNum=(int)vect.size(), p;
	for(int i=0;i<entryNum;i++){
		strcpy(msg, vect[i].c_str());
		pch=strtok(msg, "_");
		pch=strtok(NULL, "_");
		p=binarySearch(s2ci, 0, s2ciNum-1, pch);
		if(p==-1){
			printf("%s not found in %s\n", pch, syl2ci);
			exit(1);
		}
		fprintf(f, "%s\t%s\n", vect[i].c_str(), s2ci[p].dst);
	}
	fclose(f);
}

int main(int argc, char **argv){
	if(argc!=5){
		printf("Usage: %s context.in dic.char2syl dic.syl2CI confusionMatrix\n", argv[0]);
		exit(1);
	}
	char contextIn[MAXLEN], dicchar2syl[MAXLEN], dicsyl2ci[MAXLEN], confusionMatrixFile[MAXLEN];
	strcpy(contextIn, argv[1]);
	strcpy(dicchar2syl, argv[2]);
	strcpy(dicsyl2ci, argv[3]);
	strcpy(confusionMatrixFile, argv[4]);

	GENCONFUSION gc;
	gc.geConfusionEntry(contextIn, dicchar2syl, dicsyl2ci, confusionMatrixFile);

	return 0;
}
