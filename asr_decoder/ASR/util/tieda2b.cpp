#include "recog.h"

#include <string.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>

#define MIN_VAR 1E-30			// min variance value
#define MAX_VAR 1E+30			// max variance value
#define LZERO -1.0E10			// value of log(0)
#define MIN_LOG_EXP -23.02585		// Min usable log exp, MIN_LOG_EXP = -log(-LZERO)
#define MIN_MIXTURE_WEIGHT 1.0E-5	// Min usable mixture weight
#define MIN_LOG_MIXTURE_WEIGHT -11.5129254649702	// log(MIN_MIXTURE_WEIGHT)

using namespace std;

typedef struct {
	char *str;
	int tag;
} TXTINRAM;

class TIEDA2B {
public:
	TIEDA2B();
	~TIEDA2B();

	MASTERMACROFILE *MMF;
	TIEDSTATE *ts;
	TIEDMATRIX *tm;
	SYMBOLLIST *tsList, *tmList;
	TXTINRAM *tir;

	void allocateRam(char *fileName);
	void struct2List();
	void sortMMF(MASTERMACROFILE *MMF,int low,int up);
	void sortTiedMatrix(TIEDMATRIX *list, int low, int up);
	void sortTiedState(TIEDSTATE *list, int low, int up);
	int getWidth();
	int getTypeNum(const char *str);
	void setTag();
	int readMixture(int mixtureLinePos, int count, int j);
	void readTiedState();
	void readTiedMatrix();
	void readHMM();
	void modifyMacro();
	void txt2mem(char *macroFileName);
	void write2binary(char *macroBin);
	void outHMMList(char *HMMListFile);	

	int lineCount;
	int streamWidth;
	int modelNum;
	int tsNum;
	int tmNum;
};

TIEDA2B::TIEDA2B(){};

TIEDA2B::~TIEDA2B(){
	for(int i=0;i<modelNum;i++)
		delete [] MMF[i].tiedStateIndex;
	delete [] MMF;
	for(int i=0;i<tsNum;i++){
		for(int j=0;j<ts[i].mixtureNum;j++){
			delete [] ts[i].mixture[j].mean;
			delete [] ts[i].mixture[j].variance;
		}
	}
	delete [] ts;
	for(int i=0;i<tmNum;i++)
		delete [] tm[i].transProb;
	delete [] tm;
	delete [] tir;
};

void TIEDA2B::allocateRam(char *fileName){
	UTILITY util;	
	lineCount=util.getFileLineCount(fileName);

	tir = new TXTINRAM[lineCount];	
	txt2mem(fileName);
	setTag();

	streamWidth=getWidth();

	MMF = new MASTERMACROFILE[modelNum];
	ts = new TIEDSTATE[tsNum];
	tm = new TIEDMATRIX[tmNum];
	tsList = new SYMBOLLIST[tsNum];
	tmList = new SYMBOLLIST[tmNum];	
}
// set index for different information
// 1:HMM 2:state 3:transition
void TIEDA2B::setTag(){
	modelNum=0;
	tsNum=0;
	tmNum=0;

	char currentLine[MAXLEN], nextLine[MAXLEN], *p1, *p2;
	for(int i=0;i<lineCount-1;i++){
		strcpy(currentLine, tir[i].str);
		p1=strtok(currentLine, " ");
		strcpy(nextLine, tir[i+1].str);
		p2=strtok(nextLine, " ");

		if(strcmp(p1, "~h")==0){
			tir[i].tag=1;
			modelNum++;
		}
		if( (strcmp(p1, "~s")==0)&&(strcmp(p2, "<NUMMIXES>")==0) ){
			tir[i].tag=2;
			tsNum++;
		}
		if( (strcmp(p1, "~t")==0)&&(strcmp(p2, "<TRANSP>")==0) ){
			tir[i].tag=3;
			tmNum++;
		}
	}
}
// just copy the string from ts/tm to tsList/tmList
void TIEDA2B::struct2List(){
	for(int i=0;i<tsNum;i++)
		strcpy(tsList[i].str, ts[i].str);
	for(int i=0;i<tmNum;i++)
		strcpy(tmList[i].str, tm[i].str);
}

void TIEDA2B::sortMMF(MASTERMACROFILE *MMF,int low,int up){// Note:up=sizeof(array)/sizeof(char *)-1; ascending order
	// **pointer
	int HMMNum=up-low+1;
	SYMBOLLIST *HMMList = new SYMBOLLIST[HMMNum];
	for(int i=0;i<HMMNum;i++)
		strcpy(HMMList[i].str, MMF[i].str);
	// STL sort
	vector<string> vect;
	for(int i=low;i<=up;i++)
		vect.push_back(HMMList[i].str);
	sort(vect.begin(), vect.end());
	vector<string>::iterator it=unique(vect.begin(), vect.end());
	vect.erase(it, vect.end());
	// check consistency
	if(vect.size()!=HMMNum){
		printf("Some model(s) duplicated.\n");
		exit(1);
	}
	// vect -> HMMList
	for(int i=0;i<HMMNum;i++)
		strcpy(HMMList[i].str, vect[i].c_str());
	// sort MMF
	MASTERMACROFILE *sortedMMF=new MASTERMACROFILE[HMMNum];
	UTILITY util;
	int p;
	for(int i=0;i<HMMNum;i++){
		p=util.stringSearch(HMMList, 0, HMMNum-1, MMF[i].str);
		if(p==-1){
			printf("%s not found\n", MMF[i].str);
			exit(1);
		}
		sortedMMF[p]=MMF[i];
	}
	for(int i=0;i<HMMNum;i++)
		MMF[i]=sortedMMF[i];
	// release memory
	if(HMMList!=NULL){
		delete [] HMMList;
		HMMList=NULL;
	}
	if(sortedMMF!=NULL){
		delete [] sortedMMF;
		sortedMMF=NULL;
	}
}

void TIEDA2B::sortTiedMatrix(TIEDMATRIX *list, int low, int up){
	int i=low, j=up;
	char key[MAXLEN];
	TIEDMATRIX temp[1];
	strcpy(key, list[(low+up)/2].str);
	//  partition
	do{
		while (strcmp(list[i].str,key)<0) i++;
		while (strcmp(list[j].str,key)>0) j--;
		if (i<=j){
			temp[0]=list[i]; list[i]=list[j]; list[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (low<j) sortTiedMatrix(list, low, j);
	if (i<up) sortTiedMatrix(list, i, up);
}

void TIEDA2B::sortTiedState(TIEDSTATE *list, int low, int up){
	int i=low, j=up;
	char key[MAXLEN];
	TIEDSTATE temp[1];
	strcpy(key, list[(low+up)/2].str);
	//  partition
	do{
		while (strcmp(list[i].str,key)<0) i++;
		while (strcmp(list[j].str,key)>0) j--;
		if (i<=j){
			temp[0]=list[i]; list[i]=list[j]; list[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (low<j) sortTiedState(list, low, j);
	if (i<up) sortTiedState(list, i, up);
}

int TIEDA2B::getWidth(){
	char msg[MAXLEN], *pch;
	strcpy(msg, tir[1].str);
	pch=strtok(msg, " ");
	pch=strtok(NULL, " ");
	return atoi(strtok(NULL, " "));
}

int TIEDA2B::readMixture(int mixtureLinePos, int count, int j){
	char msg[MAXLEN], *pch;
	strcpy(msg, tir[mixtureLinePos].str);
	pch=strtok(msg, " ");	// <MIXTURE> 1 4.479619e-02
	pch=strtok(NULL, " ");
	pch=strtok(NULL, " ");
	ts[count].mixture[j].mixtureWeight=(float)atof(pch);
						
	ts[count].mixture[j].mean = new float[streamWidth];
	ts[count].mixture[j].variance = new float[streamWidth];

	strcpy(msg, tir[mixtureLinePos+2].str);
	pch=strtok(msg, " ");
	// mean
	for(int k=0;k<streamWidth;k++){
		ts[count].mixture[j].mean[k]=(float)atof(pch);
		pch=strtok(NULL, " ");
	}
	strcpy(msg, tir[mixtureLinePos+4].str);
	pch=strtok(msg, " ");
	// variance
	for(int k=0;k<streamWidth;k++){
		ts[count].mixture[j].variance[k]=(float)atof(pch);
		pch=strtok(NULL, " ");
	}
	strcpy(msg, tir[mixtureLinePos+5].str);
	pch=strtok(msg, " ");
	pch=strtok(NULL, " ");
	ts[count].mixture[j].gconst=(float)atof(pch);

	return mixtureLinePos+=6;	// to the line of next <MIXTURE>
}

void TIEDA2B::readTiedState(){
	int count=0, mixtureLinePos;
	char msg[MAXLEN], *pch;

	for(int i=0;i<lineCount;i++){
		if(tir[i].tag==2){
			strcpy(msg, tir[i].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");	// to get the "P1_s210"
			pch=strtok(pch, "\"");	// to get the P1_s210 without the double quote
			strcpy(ts[count].str, pch);
			
			strcpy(msg, tir[i+1].str);	// <NUMMIXES> 8
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");
			ts[count].mixtureNum=atoi(pch);
			ts[count].mixture = new MIXTURE[ts[count].mixtureNum];
			mixtureLinePos=i+2;
						
			for(int j=0;j<ts[count].mixtureNum;j++){					
				strcpy(msg, tir[mixtureLinePos].str);
				if(strcmp(strtok(msg, " "),"<MIXTURE>")==0)
					mixtureLinePos=readMixture(mixtureLinePos, count, j);
				else
					ts[count].mixture[j].mixtureWeight=0;
			}
			i=mixtureLinePos-1;
			count++;
		}
	}
	sortTiedState(ts, 0, tsNum-1);
}

void TIEDA2B::readTiedMatrix(){
	char msg[MAXLEN], *pch;
	int count=0;
	for(int i=0;i<lineCount;i++){
		if(tir[i].tag==3){
			strcpy(msg, tir[i].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");	// get "EXT_0"
			pch=strtok(pch, "\"");	// EXT_0 without double quote
			strcpy(tm[count].str, pch);
			strcpy(msg, tir[i+1].str);
			pch=strtok(msg, " ");	// <TRANSP> 5
			pch=strtok(NULL, " ");
			tm[count].stateNum=atoi(pch);
			tm[count].transProb = new float[tm[count].stateNum*tm[count].stateNum];
			for(int j=0;j<tm[count].stateNum;j++){
				strcpy(msg, tir[i+2+j].str);
				pch=strtok(msg, " ");
				for(int k=0;k<tm[count].stateNum;k++){
					tm[count].transProb[j*tm[count].stateNum+k]=(float)atof(pch);
					pch=strtok(NULL, " ");
				}
			}
			count++;
		}
	}
	sortTiedMatrix(tm, 0, tmNum-1);
}

void TIEDA2B::readHMM(){
	UTILITY util;
	int count=0, idx;
	char msg[MAXLEN], *pch;
	for(int i=0;i<lineCount;i++){
		if(tir[i].tag==1){	// ~h "P48-P18+P25"
			strcpy(msg, tir[i].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");	// "P48-P18+P25"
			pch=strtok(pch, "\"");	// P48-P18+P25 without double quote
			strcpy(MMF[count].str, pch);
			strcpy(msg, tir[i+2].str);	// <NUMSTATES> 5
			pch=strtok(msg, " ");
			MMF[count].stateNum=atoi(strtok(NULL, " "));
			MMF[count].tiedStateIndex = new int[MMF[count].stateNum];
			for(int j=0;j<MMF[count].stateNum-2;j++){
				strcpy(msg, tir[i+4+j*2].str);
				pch=strtok(msg, " "); // ~s "IH_s48"
				pch=strtok(NULL, " ");
				pch=strtok(pch, "\"");	// without double quote
				idx=util.stringSearch(tsList, 0, tsNum-1, pch);
				if(idx!=-1)
					MMF[count].tiedStateIndex[j]=idx;
				else{
					printf("%s not found in tiedState list, model %s, at line %d\n", tir[i+4+j*2].str, MMF[count].str, i);
					exit(1);
				}
			}
			strcpy(msg, tir[i+4+(MMF[count].stateNum-2)*2-1].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");
			pch=strtok(pch, "\"");	// without double quote
			idx=util.stringSearch(tmList, 0, tmNum-1, pch);
			if(idx!=-1)
				MMF[count].tiedMatrixIndex=idx;
			else{
				printf("%s not found in tiedMatrix list, at line %d\n", tir[i+4+(MMF[count].stateNum-2)*2-1].str, i);
				exit(1);
			}
			count++;
		}		
	}	
	sortMMF(MMF, 0, modelNum-1);
}
// take log of all numbers 
void TIEDA2B::modifyMacro(){
	float reciprocal;
	int i, j, k;
	for( i=0;i<tsNum;i++){
		for( j=0;j<ts[i].mixtureNum;j++){
			// take reciprocal of covariance
			for( k=0;k<streamWidth;k++){
				//printf("%d %d %d\n", i, j, k);
				if(ts[i].mixture[j].variance[k]!=0){					
					if(ts[i].mixture[j].variance[k]<MIN_VAR)
						ts[i].mixture[j].variance[k]=(float)MIN_VAR;
					if(ts[i].mixture[j].variance[k]>MAX_VAR)
						ts[i].mixture[j].variance[k]=(float)MAX_VAR;

					reciprocal=1/ts[i].mixture[j].variance[k];
					if(reciprocal<MIN_VAR)
						ts[i].mixture[j].variance[k]=(float)MIN_VAR;
					if(reciprocal>MAX_VAR)
						ts[i].mixture[j].variance[k]=(float)MAX_VAR;
					ts[i].mixture[j].variance[k]=reciprocal;
				}
				//if( i==309 && j==7 ) printf("(2) i:%d j:%d k:%d val:%f\n", i, j, k, ts[i].mixture[j].variance[k]);
			}
			// take log() of mixture weight (Excerpt from HTK)
			if(ts[i].mixture[j].mixtureWeight<MIN_MIXTURE_WEIGHT){ // 1.0E-5
				printf("Mixture weight %e < MIN_MIXTURE_WEIGHT, ignored.\n", ts[i].mixture[j].mixtureWeight);
				ts[i].mixture[j].mixtureWeight=LZERO; // -1.0E-10
			}
			else
				ts[i].mixture[j].mixtureWeight=log(ts[i].mixture[j].mixtureWeight);
		}
	}

	int dim;
	for(int i=0;i<tmNum;i++){
		dim=tm[i].stateNum;
		for(int j=0;j<dim;j++){
			for(int k=0;k<dim;k++){				
				if(tm[i].transProb[j*dim+k]!=0)
					tm[i].transProb[j*dim+k]=log(tm[i].transProb[j*dim+k]);
			}
		}
	}
}
// read ascii formatted macro file to memory
void TIEDA2B::txt2mem(char *macroFileName){
	int stringSize;
	char msg[MAXLEN], *rtn;
	UTILITY util;
	FILE *in=util.openFile(macroFileName, "rt");
	for(int i=0;i<lineCount;i++){
		rtn=fgets(msg, MAXLEN, in);
		stringSize=strlen(msg);
		msg[stringSize-1]='\0';
		tir[i].str = new char[stringSize];
		strcpy(tir[i].str, msg);
		tir[i].tag=0;
	}
	fclose(in);
}

void TIEDA2B::write2binary(char *macroBin){
	// make it a multiple of 4
	int diff=streamWidth%4, oldstreamWidth=streamWidth;
	if(diff!=0)
		streamWidth+=(4-diff);
	float dataVector[MAXLEN];

	UTILITY util;
	FILE *fp=util.openFile(macroBin, "wb");
	// write global info
	fwrite(&modelNum, sizeof(int), 1, fp);
	fwrite(&streamWidth, sizeof(int), 1, fp);
	fwrite(&tsNum, sizeof(int), 1, fp);
	fwrite(&tmNum, sizeof(int), 1, fp);
	// write model
	for(int i=0;i<modelNum;i++){
		fwrite(MMF[i].str, sizeof(char)*MODELSTRLEN, 1, fp); // WASTE OF SPACE
		fwrite(&MMF[i].stateNum, sizeof(int), 1, fp);
		fwrite(MMF[i].tiedStateIndex, sizeof(int), MMF[i].stateNum-2, fp);
		fwrite(&MMF[i].tiedMatrixIndex, sizeof(int), 1, fp);
	}
	// write tied state
	for(int i=0;i<tsNum;i++){		
		int mix[MAXLEN], mixNum=0;
		for(int m=0;m<MAXLEN;m++) // this should be upperbounded by 'ts[i].mixtureNum'??
			mix[m]=1;
		for(int j=0;j<ts[i].mixtureNum;j++) // count usable mixtures
			(ts[i].mixture[j].mixtureWeight<MIN_LOG_MIXTURE_WEIGHT)?mix[j]=0:mixNum++;

		fwrite(ts[i].str, sizeof(char)*MODELSTRLEN, 1, fp); // WASTE OF SPACE
		fwrite(&mixNum, sizeof(int), 1, fp); // WASTE OF SPACE, can use 'char'
		
		// NOTICE: it seems that the weights are not re-normalized when discarding small mixtures!
		 // BEGIN: discard useless mixture(s)
		for(int j=0;j<ts[i].mixtureNum;j++)
			if(mix[j]==1)
				fwrite(&ts[i].mixture[j].mixtureWeight, sizeof(float), 1, fp);		
		for(int j=0;j<ts[i].mixtureNum;j++){
			if(mix[j]==1){
				memset(dataVector, 0, sizeof(float)*MAXLEN); // appended 0, and should be '*streamWidth'
				for(int k=0;k<oldstreamWidth;k++)
					dataVector[k]=ts[i].mixture[j].mean[k];
				fwrite(dataVector, sizeof(float), streamWidth, fp);
			}
		}
		for(int j=0;j<ts[i].mixtureNum;j++){
			if(mix[j]==1){
				memset(dataVector, 0, sizeof(float)*MAXLEN); // appended 0, and should be '*streamWidth'
				for(int k=0;k<oldstreamWidth;k++)
					dataVector[k]=ts[i].mixture[j].variance[k];
				fwrite(dataVector, sizeof(float), streamWidth, fp);
			}
		}
		for(int j=0;j<ts[i].mixtureNum;j++)
			if(mix[j]==1)
				fwrite(&ts[i].mixture[j].gconst, sizeof(float), 1, fp);
		// END: discard useless mixture(s)
	}
	// write tied matrix
	for(int i=0;i<tmNum;i++){
		fwrite(tm[i].str, sizeof(char)*MODELSTRLEN, 1, fp);
		fwrite(&tm[i].stateNum, sizeof(int), 1, fp);
		fwrite(tm[i].transProb, sizeof(float), tm[i].stateNum*tm[i].stateNum, fp);
	}
	fclose(fp);
}

void TIEDA2B::outHMMList(char *HMMListFile){
	char msg[MAXLEN], *pch;
	UTILITY util;
	FILE *out=util.openFile(HMMListFile, "wt");
	for(int i=0;i<modelNum;i++){
		strcpy(msg, MMF[i].str);
		pch=strtok(msg, "\"");
		fprintf(out, "%s\n", pch);
	}
	fclose(out);
}


int main(int argc, char **argv) {
	char macroTxt[MAXLEN], macroBin[MAXLEN], HMMListFile[MAXLEN];
	if (argc!=4) {
		printf("Usage: %s macro.txt macro.bin HMMList\n", argv[0]);
		exit(1);
	}
	strcpy(macroTxt, argv[1]);
	strcpy(macroBin, argv[2]);
	strcpy(HMMListFile, argv[3]);

	TIEDA2B tieda2b;
	tieda2b.allocateRam(macroTxt);
	tieda2b.readTiedState();
	tieda2b.readTiedMatrix();
	tieda2b.struct2List();
	tieda2b.readHMM();
	tieda2b.modifyMacro();
	tieda2b.write2binary(macroBin);
	tieda2b.outHMMList(HMMListFile);

	return 0;
}
