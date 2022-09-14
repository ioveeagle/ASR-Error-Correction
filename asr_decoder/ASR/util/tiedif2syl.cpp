#include "recog.h"

typedef struct {
	char *str;
	int tag;
} TXTINRAM;

typedef struct {
	char SYL[MODELSTRLEN];
	char I[MODELSTRLEN];
	char F[MODELSTRLEN];
	int Iidx;
	int Fidx;
	int stateNum;
} IF2SYL;

class HMMIF2WORD {
private:
	MASTERMACROFILE *MMF;
	TIEDSTATE *ts;
	TIEDMATRIX *tm;
	SYMBOLLIST *tsList, *tmList;
	TXTINRAM *tir;

	IF2SYL *if2syl;
	int if2sylNum;

	void allocateRam(char *fileName);
	void struct2List();
	void sortMMF(MASTERMACROFILE *MMF,int low,int up);
	void sortTiedMatrix(TIEDMATRIX *tm, int low, int up);
	void sortTiedState(TIEDSTATE *ts, int low, int up);
	int getWidth();
	int getTypeNum(const char *str);
	void setTag();
	int readMixture(int mixtureLinePos, int count, int j);
	void readTiedState();
	void readTiedMatrix();
	void readHMM();
	void txt2mem(char *macroFileName);

	void readIF2syl(char *fileName);

	int lineCount;
	int streamWidth;
	int modelNum;
	int tsNum;
	int tmNum;
public:
	HMMIF2WORD();
	~HMMIF2WORD();
	void doIF2Word(char *srcHMM, char *dstHMM, char *IF2SylFile);
};

HMMIF2WORD::HMMIF2WORD(){}

HMMIF2WORD::~HMMIF2WORD(){
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

	delete [] if2syl;
}

void HMMIF2WORD::txt2mem(char *macroFileName){
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

void HMMIF2WORD::allocateRam(char *fileName){
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

int HMMIF2WORD::getWidth(){
	char msg[MAXLEN], *pch;
	strcpy(msg, tir[1].str);
	pch=strtok(msg, " ");
	pch=strtok(NULL, " ");
	return atoi(strtok(NULL, " "));
}

void HMMIF2WORD::setTag(){
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

void HMMIF2WORD::readTiedState(){
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

void HMMIF2WORD::sortTiedState(TIEDSTATE *ts, int low, int up){
	TIEDSTATE tempModel[1];
	int i,j; char *pkey;
	if(low<up){
		i=low;j=up+1;
		pkey=ts[low].str;
		do{
			do {i++;} while((strcmp(ts[i].str,pkey)<0)&&(i<up));
			do {j--;} while((strcmp(ts[j].str,pkey)>0)&&(j>low));
			if (i<j) { tempModel[0]=ts[i]; ts[i]=ts[j]; ts[j]=tempModel[0];}
		}while(i<j);
		tempModel[0]=ts[j]; ts[j]=ts[low]; ts[low]=tempModel[0];
		sortTiedState(ts,low,j-1);
		sortTiedState(ts,j+1,up);
	}
}

int HMMIF2WORD::readMixture(int mixtureLinePos, int count, int j){
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

void HMMIF2WORD::readTiedMatrix(){
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

void HMMIF2WORD::sortTiedMatrix(TIEDMATRIX *tm, int low, int up){
	TIEDMATRIX tempModel[1];
	int i,j; char *pkey;
	if(low<up){
		i=low;j=up+1;
		pkey=tm[low].str;
		do{
			do {i++;} while((strcmp(tm[i].str,pkey)<0)&&(i<up));
			do {j--;} while((strcmp(tm[j].str,pkey)>0)&&(j>low));
			if (i<j) {tempModel[0]=tm[i]; tm[i]=tm[j]; tm[j]=tempModel[0];};
		}while(i<j);
		tempModel[0]=tm[j]; tm[j]=tm[low]; tm[low]=tempModel[0];
		sortTiedMatrix(tm,low,j-1);
		sortTiedMatrix(tm,j+1,up);
	}
}

void HMMIF2WORD::readHMM(){
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
				pch=strtok(msg, " ");
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

void HMMIF2WORD::struct2List(){
	for(int i=0;i<tsNum;i++)
		strcpy(tsList[i].str, ts[i].str);
	for(int i=0;i<tmNum;i++)
		strcpy(tmList[i].str, tm[i].str);
}

void HMMIF2WORD::sortMMF(MASTERMACROFILE *MMF,int low,int up){// Note:up=sizeof(array)/sizeof(char *)-1; ascending order
	MASTERMACROFILE tempModel[1];
	int i,j; char *pkey;
	if(low<up){
		i=low;j=up+1;
		pkey=MMF[low].str;
		do{
			do {i++;} while((strcmp(MMF[i].str,pkey)<0)&&(i<up));
			do {j--;} while((strcmp(MMF[j].str,pkey)>0)&&(j>low));
			if (i<j) {tempModel[0]=MMF[i]; MMF[i]=MMF[j]; MMF[j]=tempModel[0];};
		}while(i<j);
		tempModel[0]=MMF[j]; MMF[j]=MMF[low]; MMF[low]=tempModel[0];
		sortMMF(MMF,low,j-1);
		sortMMF(MMF,j+1,up);
	}
}

void HMMIF2WORD::readIF2syl(char *fileName){
	UTILITY util;
	if2sylNum=util.getFileLineCount(fileName);
	if2syl=new IF2SYL[if2sylNum];

	FILE *in=util.openFile(fileName, "rt");
	char msg[MAXLEN], *p1, *p2, *p3;
	for(int i=0;i<if2sylNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';

		p1=strtok(msg, " ");
		p2=strtok(NULL, " ");
		p3=strtok(NULL, " ");

		strcpy(if2syl[i].SYL, p1);
		strcpy(if2syl[i].I, p2);
		strcpy(if2syl[i].F, p3);

		if2syl[i].stateNum=0;
		for(int j=0;j<modelNum;j++){
			if(strcmp(if2syl[i].I, MMF[j].str)==0){
				if2syl[i].Iidx=j;
				if2syl[i].stateNum+=(MMF[j].stateNum-2);
			}
			if(strcmp(if2syl[i].F, MMF[j].str)==0){
				if2syl[i].Fidx=j;
				if2syl[i].stateNum+=(MMF[j].stateNum-2);
			}
		}
		if2syl[i].stateNum+=2;
	}
	fclose(in);
}

void HMMIF2WORD::doIF2Word(char *srcHMM, char *dstHMM, char *IF2SylFile){
	UTILITY util;
	int stateNum, cnt, tsEntry[100];
	float prob[MAXLEN];

	// read source HMM
	allocateRam(srcHMM);
	readTiedState();
	readTiedMatrix();
	struct2List();
	readHMM();
	// read IF2syl
	readIF2syl(IF2SylFile);
	
	int nonIF[MAXLEN];
	for(int i=0;i<modelNum;i++)
		nonIF[i]=1;
	for(int i=0;i<if2sylNum;i++){
		nonIF[if2syl[i].Iidx]=0;
		nonIF[if2syl[i].Fidx]=0;
	}

	// output IF to word
	FILE *out=util.openFile(dstHMM, "wt");
	// header
	for(int i=0;i<3;i++)
		fprintf(out, "%s\n", tir[i].str);
	// tied state
	for(int i=0;i<tsNum;i++){
		fprintf(out, "~s \"%s\"\n", ts[i].str);
		fprintf(out, "<NUMMIXES> %d\n", ts[i].mixtureNum);
		for(int k=0;k<ts[i].mixtureNum;k++){
			if(ts[i].mixture[k].mixtureWeight!=0){
				fprintf(out, "<MIXTURE> %d %e\n", k+1, ts[i].mixture[k].mixtureWeight);
				fprintf(out, "<MEAN> %d\n", streamWidth);
				for(int m=0;m<streamWidth;m++)
					fprintf(out, " %e", ts[i].mixture[k].mean[m]);
				fprintf(out, "\n");
				fprintf(out, "<VARIANCE> %d\n", streamWidth);
				for(int m=0;m<streamWidth;m++)
					fprintf(out, " %e", ts[i].mixture[k].variance[m]);
				fprintf(out, "\n");
				fprintf(out, "<GCONST> %e\n", ts[i].mixture[k].gconst);
			}
		}
	}
	// tied matrix
	for(int i=0;i<if2sylNum;i++){
		fprintf(out, "~t \"EXT_W%d\"\n", i);
		fprintf(out, "<TRANSP> %d\n", if2syl[i].stateNum);
		memset(prob, 0, sizeof(float)*MAXLEN);
		int p=if2syl[i].Iidx;
		prob[1]=1;
		cnt=if2syl[i].stateNum;
		for(int j=1;j<MMF[p].stateNum-1;j++){
			for(int k=0;k<MMF[p].stateNum;k++)
				prob[cnt+k]=tm[MMF[p].tiedMatrixIndex].transProb[j*MMF[p].stateNum+k];
			cnt+=if2syl[i].stateNum;
		}
		p=if2syl[i].Fidx;
		cnt+=2;
		for(int j=1;j<MMF[p].stateNum-1;j++){
			for(int k=0;k<MMF[p].stateNum;k++)
				prob[cnt+k]=tm[MMF[p].tiedMatrixIndex].transProb[j*MMF[p].stateNum+k];
			cnt+=if2syl[i].stateNum;
		}
		stateNum=if2syl[i].stateNum;
		for(int j=0;j<stateNum;j++){
			for(int k=0;k<stateNum;k++)
				fprintf(out, " %e", prob[j*stateNum+k]);
			fprintf(out, "\n");
		}
	}
	for(int i=0;i<modelNum;i++){
		if(nonIF[i]==1){
			fprintf(out, "~t \"EXT_%d\"\n", i);
			fprintf(out, "<TRANSP> %d\n", MMF[i].stateNum);
			stateNum=MMF[i].stateNum;
			for(int j=0;j<stateNum;j++){
				for(int k=0;k<stateNum;k++)
					fprintf(out, " %e", tm[MMF[i].tiedMatrixIndex].transProb[j*stateNum+k]);
				fprintf(out, "\n");
			}
		}
	}
	// model
	for(int i=0;i<if2sylNum;i++){
		fprintf(out, "~h \"%s\"\n", if2syl[i].SYL);
		// state
		cnt=0;
		for(int k=0;k<MMF[if2syl[i].Iidx].stateNum-2;k++){
			tsEntry[cnt]=MMF[if2syl[i].Iidx].tiedStateIndex[k];
			cnt++;
		}
		for(int k=0;k<MMF[if2syl[i].Fidx].stateNum-2;k++){
			tsEntry[cnt]=MMF[if2syl[i].Fidx].tiedStateIndex[k];
			cnt++;
		}
		fprintf(out, "<BEGINHMM>\n");
		fprintf(out, "<NUMSTATES> %d\n", if2syl[i].stateNum);
		for(int j=0;j<if2syl[i].stateNum-2;j++){
			fprintf(out, "<STATE> %d\n", j+2); // counted from 2
			fprintf(out, "~s \"%s\"\n", ts[tsEntry[j]].str);
		}
		// transition matrix
		fprintf(out, "~t \"EXT_W%d\"\n", i);
		fprintf(out, "<ENDHMM>\n");
	}
	for(int i=0;i<modelNum;i++){
		if(nonIF[i]==1){
			fprintf(out, "~h \"%s\"\n", MMF[i].str);
			fprintf(out, "<BEGINHMM>\n");
			fprintf(out, "<NUMSTATES> %d\n", MMF[i].stateNum);
			for(int j=0;j<MMF[i].stateNum-2;j++){
				fprintf(out, "<STATE> %d\n", j+2); // counted from 2
				fprintf(out, "~s \"%s\"\n", ts[MMF[i].tiedStateIndex[j]].str);
			}
			// transition matrix
			fprintf(out, "~t \"EXT_%d\"\n", i);
			fprintf(out, "<ENDHMM>\n");
		}
	}

	fclose(out);
}

int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage: %s IF.tiedstate outFile IF2syl\n", argv[0]);
		printf("It converts tiedstate-IF to tiedstate.word model\n");
		exit(1);
	}

	HMMIF2WORD if2w;
	if2w.doIF2Word(argv[1], argv[2], argv[3]);

	return 0;
}
