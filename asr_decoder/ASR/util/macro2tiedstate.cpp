#include "recog.h"
#define MAX_DIMENSION 40
#define MAX_MIXTURE_NUM	64

typedef struct {
	int extractMark;
	char *str;
} RAMDATA;

typedef struct {
	char str[MODELSTRLEN];
	float mixtureWeight;
	float mean[MAX_DIMENSION];
	float variance[MAX_DIMENSION];
	float gconst;
} EXTRACTEDMIXTURE;

typedef struct {
	char str[MODELSTRLEN];
	int stateNum;
	float prob[(MAX_STATE_NUM+2)*(MAX_STATE_NUM+2)];
} EXTRACTEDMATRIX;

typedef struct {
	char str[MODELSTRLEN];
	int mixtureNum;
	int mixture[MAX_MIXTURE_NUM];
	float mixtureWeight[MAX_MIXTURE_NUM];
} EXTRACTEDSTATE;

typedef struct {
	char str[MODELSTRLEN];
	int stateNum;
	int state[MAX_STATE_NUM];
	char extractedMatrix[MODELSTRLEN];
} EXTRACTEDMODEL;

class EXTRACTMACRO {
private:
	int modelType;//0: tied mixture, 1: tied state, 2: regular
	int dimension;
	RAMDATA *srcData;
	int srcDataLineNum;
	EXTRACTEDMIXTURE *extractedMixture;
	int extractedMixtureNum;
	EXTRACTEDMATRIX *extractedMatrix;
	int extractedMatrixNum;
	EXTRACTEDSTATE *extractedState;
	int extractedStateNum;
	EXTRACTEDMODEL *extractedModel;
	int extractedModelNum;
	void readFileIntoRam(char *inFile);
	void readMixture(EXTRACTEDMIXTURE *em, int emId, int line, float weight);
	void obtainTiedMixture();
	void obtainTiedState();
	void obtainTiedMatrix();
	void groupeMixture2State();
	void tiedStateFormat2Model();
	void copy2srcData();
	void checkMacroFormat();
	void checkSingleMixture();
	void setMark();
	char *getQuotedString(char *inStr);
	void setRemoveMark(RAMDATA *r, int s, int e);
	void updatesrcData();
	void sortTiedMixture(EXTRACTEDMIXTURE *t, int low, int up);
	int searchMixture(EXTRACTEDMIXTURE *t, int left, int right, char *inputstr);
	void sortTiedState(EXTRACTEDSTATE *t, int low, int up);
	int searchState(EXTRACTEDSTATE *t, int left, int right, char *inputstr);
	void outputTiedStateMacro(char *outFile);
public:
	EXTRACTMACRO();
	~EXTRACTMACRO();	
	void doExtractMacro(char *inFile, char *outFile);
};

EXTRACTMACRO::EXTRACTMACRO(){
}

EXTRACTMACRO::~EXTRACTMACRO(){
	if(extractedMixture!=NULL)
		delete [] extractedMixture;
	if(extractedMatrix!=NULL)
		delete [] extractedMatrix;
	if(extractedState!=NULL)
		delete [] extractedState;
	if(extractedModel!=NULL)
		delete [] extractedModel;
	if(srcData!=NULL){
		for(int i=0;i<srcDataLineNum;i++)
			delete [] srcData[i].str;
		delete [] srcData;
	}
}

int EXTRACTMACRO::searchMixture(EXTRACTEDMIXTURE *t, int left, int right, char *inputstr){
	int mid=(left+right)/2,difference;
	difference=strcmp(inputstr,t[mid].str);
	if (difference==0)
		return mid;
	(difference>0)?left=mid+1:right=mid-1;
	return ((left>right)||(right<left))?-1:searchMixture(t,left,right,inputstr);
}

int EXTRACTMACRO::searchState(EXTRACTEDSTATE *t, int left, int right, char *inputstr){
	int mid=(left+right)/2,difference;
	difference=strcmp(inputstr,t[mid].str);
	if (difference==0)
		return mid;
	(difference>0)?left=mid+1:right=mid-1;
	return ((left>right)||(right<left))?-1:searchState(t,left,right,inputstr);
}

void EXTRACTMACRO::checkMacroFormat(){
	char msg[MAXLEN], *pch;
	strcpy(msg, srcData[1].str);
	pch=strtok(msg, " ");
	pch=strtok(NULL, " ");
	pch=strtok(NULL, " ");
	dimension=atoi(pch);

	int foundS=0, foundM=0;
	for(int i=0;i<srcDataLineNum;i++){
		strcpy(msg, srcData[i].str);
		pch=strtok(msg, " ");
		if(strcmp(pch, "~s")==0){ // found tied-state
			foundS=1;
			break;
		}
	}
	for(int i=0;i<srcDataLineNum;i++){
		strcpy(msg, srcData[i].str);
		pch=strtok(msg, " ");
		if(strcmp(pch, "~m")==0){ // found tied-mixture
			foundM=1;
			break;
		}
	}
	if((foundS==0)&&(foundM==1))
		modelType=0;
	if((foundS==1)&&(foundM==0))
		modelType=1;
	if((foundS==0)&&(foundM==0))
		modelType=2;
	if((foundS==1)&&(foundM==1)){
		printf("Mix of tied-state and tied-mixture not supported.\n");
		exit(1);
	}
}

char *EXTRACTMACRO::getQuotedString(char *inStr){
	char msg[MAXLEN], *pch;
	strcpy(msg, inStr);
	pch=strtok(msg, " ");
	pch=strtok(NULL, " ");
	pch=strtok(pch, "\"");
	if(strlen(pch)>=MODELSTRLEN){
		printf("length of %s is over MODELSTRLEN %d\n", pch, MODELSTRLEN);
		exit(1);
	}
	return pch;
}

void EXTRACTMACRO::checkSingleMixture(){
	int addedLimit=1000, addedCount=0;
	RAMDATA *tmp = new RAMDATA[srcDataLineNum+addedLimit];

	int c=0, i;
	char currentLine[MAXLEN], nextLine[MAXLEN], *p1, *p2;
	for(i=0;i<srcDataLineNum-1;i++){	
		tmp[c]=srcData[i];
		c++;
		strcpy(currentLine, srcData[i].str);
		p1=strtok(currentLine, " ");
		strcpy(nextLine, srcData[i+1].str);
		p2=strtok(nextLine, " ");
		if((strcmp(p1, "<STATE>")==0)&&(strcmp(p2, "<NUMMIXES>")!=0)&&(strcmp(p2, "~s")!=0)){
			//printf("i=%d\tcl=%s\ttmp[c].str=%s\tsrcData[i]=%s\n", i, currentLine, tmp[c].str, srcData[i].str);
			tmp[c].str=new char[MAXLEN];
			strcpy(tmp[c].str, "<NUMMIXES> 1");
			tmp[c].extractMark=0;
			c++;

			tmp[c].str=new char[MAXLEN];
			strcpy(tmp[c].str, "<MIXTURE> 1 1.000000e-00");
			tmp[c].extractMark=0;
			c++;

			addedCount++;
			if(addedCount==addedLimit){
				printf("Increase addedLimit %d\n", addedLimit);
				exit(1);
			}
		}
	}
	//printf("addedCount:%d\n", addedCount);

	tmp[c]=srcData[srcDataLineNum-1];
	c++;

	if(c==srcDataLineNum)// no tag lost
		delete [] tmp;
	else{
		if(srcData!=NULL)
			delete [] srcData;
		srcDataLineNum=c;
		srcData = new RAMDATA[srcDataLineNum];
		for(int i=0;i<srcDataLineNum;i++)
			srcData[i]=tmp[i];
		if(tmp!=NULL)
			delete [] tmp;
	}
}

void EXTRACTMACRO::readFileIntoRam(char *inFile){
	UTILITY util;
	srcDataLineNum=util.getFileLineCount(inFile);
	srcData = new RAMDATA[srcDataLineNum];

	FILE *in=util.openFile(inFile, "rt");
	char msg[MAXLEN], *rtn;
	int len;
	for(int i=0;i<srcDataLineNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		len=strlen(msg);
		srcData[i].str=new char[len+1];
		strcpy(srcData[i].str, msg);
		srcData[i].extractMark=0;
	}
	fclose(in);

	checkMacroFormat();
	checkSingleMixture();
	setMark();
}

void EXTRACTMACRO::updatesrcData(){
	RAMDATA *tmp = new RAMDATA[srcDataLineNum];
	// srcData -> tmp
	int c=0;
	for(int i=0;i<srcDataLineNum;i++){
		if(srcData[i].str[0]!='#'){
			tmp[c].str=new char[strlen(srcData[i].str)+1];
			tmp[c].extractMark=srcData[i].extractMark;
			strcpy(tmp[c].str, srcData[i].str);
			c++;
		}
	}
	// delete srcData
	if(srcData!=NULL){
		for(int i=0;i<srcDataLineNum;i++){
			delete [] srcData[i].str;
			srcData[i].str=NULL;			
		}
		delete [] srcData;
		srcData=NULL;
	}
	// copy tmp to srcData
	srcData = new RAMDATA[c];
	for(int i=0;i<c;i++){
		srcData[i].extractMark=tmp[i].extractMark;
		srcData[i].str=new char[strlen(tmp[i].str)+1];		
		strcpy(srcData[i].str, tmp[i].str);
	}
	srcDataLineNum=c;
	// delete tmp
	if(tmp!=NULL){
		for(int i=0;i<c;i++){
			delete [] tmp[i].str;
			tmp[i].str=NULL;			
		}
		delete [] tmp;
		tmp=NULL;
	}
}

void EXTRACTMACRO::readMixture(EXTRACTEDMIXTURE *em, int emId, int line, float weight){
	char msg[MAXLEN], *pch;
	// weight
	extractedMixture[emId].mixtureWeight=weight;
	// mean
	strcpy(msg, srcData[line+2].str);
	pch=strtok(msg, " ");
	extractedMixture[emId].mean[0]=(float)atof(pch);
	for(int j=1;j<dimension;j++){
		pch=strtok(NULL, " ");
		extractedMixture[emId].mean[j]=(float)atof(pch);
	}
	// variance
	strcpy(msg, srcData[line+4].str);
	pch=strtok(msg, " ");
	extractedMixture[emId].variance[0]=(float)atof(pch);
	for(int j=1;j<dimension;j++){
		pch=strtok(NULL, " ");
		extractedMixture[emId].variance[j]=(float)atof(pch);
	}
	// gconst
	strcpy(msg, srcData[line+5].str);
	pch=strtok(msg, " ");
	pch=strtok(NULL, " ");
	extractedMixture[emId].gconst=(float)atof(pch);
}

void EXTRACTMACRO::setRemoveMark(RAMDATA *r, int s, int e){
	for(int i=s;i<=e;i++)
		strcpy(r[i].str, "#");
}

void EXTRACTMACRO::obtainTiedMixture(){
	// allocate ram
	extractedMixture = new EXTRACTEDMIXTURE[extractedMixtureNum];
	// extract mixture
	int addedMix=0, m=0, cleanStart=-1, cleanEnd=-1;
	char msg[MAXLEN], *pch;
	float mixtureWeight=0;
	for(int i=0;i<srcDataLineNum;i++){
		if((srcData[i].extractMark==1)||(srcData[i].extractMark==2)){
			if(srcData[i].extractMark==1){ // ~m "mixtureName"
				strcpy(extractedMixture[m].str, getQuotedString(srcData[i].str));
				mixtureWeight=0;
				cleanStart=i;
			}
			if(srcData[i].extractMark==2){ // <MIXTURE> mId mixWeight
				sprintf(msg, "EXM_%d", addedMix);
				strcpy(extractedMixture[m].str, msg);
				addedMix++;
				strcpy(msg, srcData[i].str);
				pch=strtok(msg, " ");
				pch=strtok(NULL, " ");
				pch=strtok(NULL, " ");
				mixtureWeight=(float)atof(pch);
				cleanStart=i+2;
			}
			cleanEnd=i+5;

			readMixture(extractedMixture, m, i, mixtureWeight);
			// assign a mixture name
			if(srcData[i].extractMark==2){
				sprintf(msg, "~m \"%s\"", extractedMixture[m].str);
				strcpy(srcData[i+1].str, msg);
			}
			setRemoveMark(srcData, cleanStart, cleanEnd);

			m++;
		}
	}
	sortTiedMixture(extractedMixture, 0, extractedMixtureNum-1);
	updatesrcData();
}

void EXTRACTMACRO::obtainTiedState(){
	// allocate memory
	extractedState = new EXTRACTEDSTATE[extractedStateNum];
	extractedMixture = new EXTRACTEDMIXTURE[extractedMixtureNum];
	// extract state
	int s=0, addedState=0, m=0, mixId, mixtureNum, clearStart=-1, clearEnd=-1, pos;
	float mixtureWeight;
	char msg[MAXLEN], *pch;
	for(int i=0;i<srcDataLineNum;i++){
		if((srcData[i].extractMark==5)||(srcData[i].extractMark==6)){
			if(srcData[i].extractMark==5){ // ~s + <NUMMIXES>
				strcpy(extractedState[s].str, getQuotedString(srcData[i].str));
				clearStart=i;
			}
			if(srcData[i].extractMark==6){ // <STATE> + <NUMMIXES>
				sprintf(msg, "EXS_%d", addedState);
				strcpy(extractedState[s].str, msg);
				addedState++;
				clearStart=i+2; // i+1 is reserved for ~s EXS_
			}
			// <NUMMIXES> 16
			strcpy(msg, srcData[i+1].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");
			mixtureNum=atoi(pch);
			extractedState[s].mixtureNum=mixtureNum;
			if(mixtureNum>MAX_MIXTURE_NUM){
				printf("line %d, mixtureNum exceeds MAX_MIXTURE_NUM %d\n", i+1, MAX_MIXTURE_NUM);
				exit(1);
			}
			// <MIXTURE> 1 1.377270e-02
			pos=i+2;
			strcpy(msg, srcData[pos].str);
			pch=strtok(msg, " ");
			while(strcmp(pch, "<MIXTURE>")==0){
				pch=strtok(NULL, " ");
				mixId=atoi(pch)-1; // counted from 0
				pch=strtok(NULL, " ");
				mixtureWeight=(float)atof(pch);
				readMixture(extractedMixture, m, pos, mixtureWeight);
				extractedState[s].mixture[mixId]=m;
				extractedState[s].mixtureWeight[mixId]=mixtureWeight;
				m++;

				pos+=6;
				strcpy(msg, srcData[pos].str);
				pch=strtok(msg, " ");
			}
			// assign tied-state
			if(srcData[i].extractMark==6){
				sprintf(msg, "~s \"%s\"", extractedState[s].str);
				strcpy(srcData[i+1].str, msg);
			}
			clearEnd=pos-1;
			setRemoveMark(srcData, clearStart, clearEnd);

			s++;
		}
	}
	sortTiedState(extractedState, 0, extractedStateNum-1);
	updatesrcData();
}

void EXTRACTMACRO::sortTiedMixture(EXTRACTEDMIXTURE *t, int low, int up){
	int i=low, j=up;
	char key[MAXLEN];
	EXTRACTEDMIXTURE temp[1];
	strcpy(key, t[(low+up)/2].str);
	//  partition
	do{
		while (strcmp(t[i].str,key)<0) i++;
		while (strcmp(t[j].str,key)>0) j--;
		if (i<=j){
			temp[0]=t[i]; t[i]=t[j]; t[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);

	//  recursion
	if (low<j) sortTiedMixture(t, low, j);
	if (i<up) sortTiedMixture(t, i, up);
}

void EXTRACTMACRO::sortTiedState(EXTRACTEDSTATE *t, int low, int up){
	int i=low, j=up;
	char key[MAXLEN];
	EXTRACTEDSTATE temp[1];
	strcpy(key, t[(low+up)/2].str);
	//  partition
	do{
		while (strcmp(t[i].str,key)<0) i++;
		while (strcmp(t[j].str,key)>0) j--;
		if (i<=j){
			temp[0]=t[i]; t[i]=t[j]; t[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);

	//  recursion
	if (low<j) sortTiedState(t, low, j);
	if (i<up) sortTiedState(t, i, up);
}

void EXTRACTMACRO::setMark(){
	extractedModelNum=0;
	extractedMixtureNum=0;
	extractedMatrixNum=0;
	extractedStateNum=0;

	char *p1, *p2, currentLine[MAXLEN], nextLine[MAXLEN];
	for(int i=0;i<srcDataLineNum-1;i++){
		strcpy(currentLine, srcData[i].str);
		p1=strtok(currentLine, " ");
		strcpy(nextLine, srcData[i+1].str);
		p2=strtok(nextLine, " ");
		// mixture info
		if( (strcmp(p1, "~m")==0)&&(strcmp(p2, "<MEAN>")==0) ){
			srcData[i].extractMark=1;
			i+=5;
			extractedMixtureNum++;
		}
		if( (strcmp(p1, "<MIXTURE>")==0)&&(strcmp(p2, "<MEAN>")==0) ){
			srcData[i].extractMark=2;
			i+=5;
			extractedMixtureNum++;
		}
		// transition matrix info
		if(strcmp(p1, "<TRANSP>")==0){
			srcData[i].extractMark=3;
			extractedMatrixNum++;
		}
		// model info
		if(strcmp(p1, "~h")==0){
			srcData[i].extractMark=4;
			extractedModelNum++;
		}
		// state info
		if( (strcmp(p1, "~s")==0)&&(strcmp(p2, "<NUMMIXES>")==0) ){
			srcData[i].extractMark=5;
			extractedStateNum++;
		}
		if( (strcmp(p1, "<STATE>")==0)&&(strcmp(p2, "<NUMMIXES>")==0) ){
			srcData[i].extractMark=6;
			extractedStateNum++;
		}
	}
}

void EXTRACTMACRO::obtainTiedMatrix(){
	extractedMatrix = new EXTRACTEDMATRIX[extractedMatrixNum];

	char *pch, msg[MAXLEN];
	int stateNum, m=0, c;
	for(int i=0;i<srcDataLineNum;i++){
		if(srcData[i].extractMark==3){ // <TRANSP>
			strcpy(msg, srcData[i].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");
			stateNum=atoi(pch);
			if(stateNum-2>MAX_STATE_NUM){
				printf("line %d, stateNum exceeds %d\n", i+1, MAX_STATE_NUM);
				exit(1);
			}			
			extractedMatrix[m].stateNum=stateNum; // in ASUS case, 5
			// string of transition matrix
			if((srcData[i-1].str[0]=='~')&&(srcData[i-1].str[1]=='t')){
				strcpy(msg, srcData[i-1].str);
				pch=strtok(msg, "\"");
				pch=strtok(NULL, "\"");
				strcpy(extractedMatrix[m].str, pch);
			}
			else{
				strcpy(extractedMatrix[m].str, "EXT_");
				sprintf(msg, "%d", m);
				strcat(extractedMatrix[m].str, msg);
			}
			// prob of transition matrix
			c=0;
			for(int s=0;s<stateNum;s++){
				strcpy(msg, srcData[i+1+s].str);
				pch=strtok(msg, " ");
				while(pch!=NULL){
					extractedMatrix[m].prob[c]=(float)atof(pch);
					c++;
					pch=strtok(NULL, " ");
				}
			}
			// replaced by ~t
			sprintf(msg, "~t \"%s\"", extractedMatrix[m].str);
			delete [] srcData[i].str;
			srcData[i].str=NULL;
			srcData[i].str=new char[strlen(msg)+1];
			strcpy(srcData[i].str, msg);

			setRemoveMark(srcData, i+1, i+stateNum); // replace extracted lines by "#"

			m++;
			i+=stateNum;
		}
	}
	updatesrcData(); // remove lines with "#" as header
}

void EXTRACTMACRO::groupeMixture2State(){
	extractedState = new EXTRACTEDSTATE[extractedStateNum];

	float weight;
	int addedState=0, mixtureNum, m, cleanStart, cleanEnd, mId;
	char msg[MAXLEN], *pch;
	for(int i=0;i<srcDataLineNum;i++){
		if(srcData[i].extractMark==6){
			// mixtureNum
			strcpy(msg, srcData[i+1].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");
			mixtureNum=atoi(pch);
			extractedState[addedState].mixtureNum=mixtureNum;
			if(mixtureNum>MAX_MIXTURE_NUM){
				printf("line %d, mixtureNum exceeds MAX_MIXTURE_NUM %d\n", i+1, MAX_MIXTURE_NUM);
				exit(1);
			}
			memset(extractedState[addedState].mixtureWeight, 0, sizeof(float)*mixtureNum);
			// assign ~s "name"
			sprintf(msg, "EXS_%d", addedState);
			strcpy(extractedState[addedState].str, msg);
			strcpy(srcData[i+1].str, "~s \"");
			strcat(srcData[i+1].str, msg);
			strcat(srcData[i+1].str, "\"");			
			// obtain mixture info
			cleanStart=i+2;
			cleanEnd=cleanStart;
			strcpy(msg, srcData[cleanEnd].str);
			pch=strtok(msg, " ");
			while(strcmp(pch, "<MIXTURE>")==0){
				//<MIXTURE> 1 1.377270e-02
				pch=strtok(NULL, " ");
				m=atoi(pch)-1; // counted from 0
				pch=strtok(NULL, " ");
				weight=(float)atof(pch);
				cleanEnd++;
				// ~m "m1_2_2_1_1_1"
				strcpy(msg, getQuotedString(srcData[cleanEnd].str));
				mId=searchMixture(extractedMixture, 0, extractedMixtureNum-1, msg);
				if(mId==-1){
					printf("%s not found in extractedMixture at line %d\n", srcData[cleanEnd].str, cleanEnd);
					exit(1);
				}
				extractedState[addedState].mixture[m]=mId;
				extractedState[addedState].mixtureWeight[m]=weight;

				cleanEnd++;
				strcpy(msg, srcData[cleanEnd].str);
				pch=strtok(msg, " ");
			}
			cleanEnd--;
			setRemoveMark(srcData, cleanStart, cleanEnd);

			addedState++;
			i=cleanEnd-1;
		}
	}
	sortTiedState(extractedState, 0, extractedStateNum-1);
	updatesrcData();
}

void EXTRACTMACRO::tiedStateFormat2Model(){
	extractedModel = new EXTRACTEDMODEL[extractedModelNum];

	char *pch, msg[MAXLEN];
	int m=0, exId;
	for(int i=0;i<srcDataLineNum;i++){
		if(srcData[i].extractMark==4){
			// ~h "modelName"
			strcpy(extractedModel[m].str, getQuotedString(srcData[i].str));
			// <BEGINHMM>
			strcpy(msg, srcData[i+1].str);
			// <NUMSTATES> 5
			strcpy(msg, srcData[i+2].str);
			pch=strtok(msg, " ");
			pch=strtok(NULL, " ");
			extractedModel[m].stateNum=atoi(pch)-2;
			// obtain state
			for(int j=0;j<extractedModel[m].stateNum;j++){
				// <STATE> 2
				strcpy(msg, srcData[i+3+j*2].str);
				// ~s "P1_s210"
				strcpy(msg, getQuotedString(srcData[i+4+j*2].str));
				exId=searchState(extractedState, 0, extractedStateNum-1, msg);
				if(exId==-1){
					printf("%s (%s) not found in extractedState at line %d\n", srcData[i+4+j*2].str, extractedModel[m].str, i);
					exit(1);
				}
				extractedModel[m].state[j]=exId;
			}
			// ~t "EXT_#"
			strcpy(extractedModel[m].extractedMatrix, getQuotedString(srcData[i+extractedModel[m].stateNum*2+3].str));

			m++;
		}
	}
}

void EXTRACTMACRO::outputTiedStateMacro(char *outFile){
	UTILITY util;
	FILE *out=util.openFile(outFile, "wt");
	// header
	for(int i=0;i<3;i++)
		fprintf(out, "%s\n", srcData[i].str);
	// tied state
	int mixId;
	for(int i=0;i<extractedStateNum;i++){
		fprintf(out, "~s \"%s\"\n", extractedState[i].str);
		fprintf(out, "<NUMMIXES> %d\n", extractedState[i].mixtureNum);
		for(int k=0;k<extractedState[i].mixtureNum;k++){
			if(extractedState[i].mixtureWeight[k]!=0){
				mixId=extractedState[i].mixture[k];
				fprintf(out, "<MIXTURE> %d %e\n", k+1, extractedState[i].mixtureWeight[k]);
				fprintf(out, "<MEAN> %d\n", dimension);					
				for(int m=0;m<dimension;m++)
					fprintf(out, " %e", extractedMixture[mixId].mean[m]);
				fprintf(out, "\n");
				fprintf(out, "<VARIANCE> %d\n", dimension);
				for(int m=0;m<dimension;m++)
					fprintf(out, " %e", extractedMixture[mixId].variance[m]);
				fprintf(out, "\n");
				fprintf(out, "<GCONST> %e\n", extractedMixture[mixId].gconst);
			}
		}
	}
	// tied matrix
	int stateNum;
	for(int i=0;i<extractedMatrixNum;i++){
		fprintf(out, "~t \"%s\"\n", extractedMatrix[i].str);
		fprintf(out, "<TRANSP> %d\n", extractedMatrix[i].stateNum);
		stateNum=extractedMatrix[i].stateNum;
		for(int j=0;j<stateNum;j++){
			for(int k=0;k<stateNum;k++)
				fprintf(out, " %e", extractedMatrix[i].prob[j*stateNum+k]);
			fprintf(out, "\n");
		}
	}
	// model
	for(int i=0;i<extractedModelNum;i++){
		stateNum=extractedModel[i].stateNum;
		fprintf(out, "~h \"%s\"\n", extractedModel[i].str);
		fprintf(out, "<BEGINHMM>\n");
		fprintf(out, "<NUMSTATES> %d\n", stateNum+2);
		for(int j=0;j<stateNum;j++){
			fprintf(out, "<STATE> %d\n", j+2); // counted from 2
			fprintf(out, "~s \"%s\"\n", extractedState[extractedModel[i].state[j]].str);
		}
		fprintf(out, "~t \"%s\"\n", extractedModel[i].extractedMatrix);
		fprintf(out, "<ENDHMM>\n");
	}
	fclose(out);
}

void EXTRACTMACRO::doExtractMacro(char *inFile, char *outFile){
	readFileIntoRam(inFile);
	printf("model type is %d\n", modelType);
	obtainTiedMatrix();

	if(modelType==0){
		obtainTiedMixture();
		groupeMixture2State();
	}
	if((modelType==1)||(modelType==2))
		obtainTiedState();

	printf("tiedStateFormat2Model()\n");
	tiedStateFormat2Model();
	printf("outputTiedStateMacro()\n");
	outputTiedStateMacro(outFile);
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s inMacro outMacro\n", argv[0]);
		printf("Convert tied-mixture/tied-state/regular macro to tied-state format.\n");
		exit(1);
	}

	char inMacro[MAXLEN], outMacro[MAXLEN];
	strcpy(inMacro, argv[1]);
	strcpy(outMacro, argv[2]);

	EXTRACTMACRO em;
	em.doExtractMacro(inMacro, outMacro);

	return 0;
}
