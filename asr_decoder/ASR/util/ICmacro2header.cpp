#include "recog.h"

#define IC_SCALE_MIXWEIGHT	1024
#define IC_SCALE_MEAN	1024
#define IC_SCALE_VARIANCE	64	// reciprocal*SCALE_VARIANCE, v -> (1/v), (1/v)*64
#define IC_SCALE_GCONST	1024

class ICMACRO2HEADER{
private:
	// IC
	int stateNum, mixtureNum, vectorDim;
	int *HMMMixturePos, *HMMMeanVariancePos, *HMMStateMixtureNum, *HMMMixtureWeight, *HMMMean, *HMMVariance, *HMMStateNum, *HMMState, *HMMStatePos;
	int *HMMGconst;
	// function
	int elimiateZeroValue();
	void macro2vector();
	void checkBoundary(float *dataVector, int vectorSize, int scale, int dataType);
	MACRO M;
public:
	ICMACRO2HEADER();
	~ICMACRO2HEADER();
	void doMacro2Header(char *inMacro, char *outFile);
};

ICMACRO2HEADER::ICMACRO2HEADER(){}

ICMACRO2HEADER::~ICMACRO2HEADER(){
	if(HMMMixtureWeight!=NULL){
		delete [] HMMMixtureWeight;
		HMMMixtureWeight=NULL;
	}
	if(HMMMean!=NULL){
		delete [] HMMMean;
		HMMMean=NULL;
	}
	if(HMMVariance!=NULL){
		delete [] HMMVariance;
		HMMVariance=NULL;
	}
	if(HMMStateNum!=NULL){
		delete [] HMMStateNum;
		HMMStateNum=NULL;
	}
	if(HMMState!=NULL){
		delete [] HMMState;
		HMMState=NULL;
	}
	if(HMMGconst!=NULL){
		delete [] HMMGconst;
		HMMGconst=NULL;
	}
	if(HMMMixturePos!=NULL){
		delete [] HMMMixturePos;
		HMMMixturePos=NULL;
	}
	if(HMMMeanVariancePos!=NULL){
		delete [] HMMMeanVariancePos;
		HMMMeanVariancePos=NULL;
	}
	if(HMMStateMixtureNum!=NULL){
		delete [] HMMStateMixtureNum;
		HMMStateMixtureNum=NULL;
	}
	if(HMMStatePos!=NULL){
		delete [] HMMStatePos;
		HMMStatePos=NULL;
	}
	// Macro
	M.ts=NULL;
	M.tm=NULL;
	M.p2ts=NULL;
	M.p2tm=NULL;
	M.p2MMF=NULL;
	M.MMF=NULL;
}

int ICMACRO2HEADER::elimiateZeroValue(){
	// get real stream width
	int width=0;
	for(int i=0;i<M.streamWidth;i++){
		if((M.ts[0].mean[i]!=0)&&(M.ts[0].variance[i]!=0))
			width++;
	}
	// move 0 values
	UTILITY util;
	for(int i=0;i<M.tsNum;i++){
		int cnt, len;
		float *tmp=NULL;
		len=M.streamWidth*M.ts[i].mixtureNum;
		// mean		
		cnt=0;
		tmp=new float[len];
		for(int j=0;j<len;j++){
			if(M.ts[i].mean[j]!=0){
				tmp[cnt]=M.ts[i].mean[j];
				cnt++;
			}
		}
		for(int j=0;j<cnt;j++)
			M.ts[i].mean[j]=tmp[j];
		if(tmp!=NULL){
			delete [] tmp;
			tmp=NULL;
		}
		// variance
		cnt=0;
		tmp=new float[len];
		for(int j=0;j<len;j++){
			if(M.ts[i].variance[j]!=0){
				tmp[cnt]=M.ts[i].variance[j];
				cnt++;
			}
		}
		for(int j=0;j<cnt;j++)
			M.ts[i].variance[j]=tmp[j];
		if(tmp!=NULL){
			delete [] tmp;
			tmp=NULL;
		}
	}
	return width;
}

void ICMACRO2HEADER::macro2vector(){
	int c, d;
	// check boundary
	for(int i=0;i<M.tsNum;i++){
		for(int j=0;j<M.ts[i].mixtureNum;j++){
			checkBoundary(M.ts[i].mean+j*vectorDim, vectorDim, IC_SCALE_MEAN, 0);
			checkBoundary(M.ts[i].variance+j*vectorDim, vectorDim, IC_SCALE_VARIANCE, 0);
		}
		checkBoundary(M.ts[i].mixtureWeight, M.ts[i].mixtureNum, IC_SCALE_MIXWEIGHT, 0);
		checkBoundary(M.ts[i].gconst, M.ts[i].mixtureNum, IC_SCALE_GCONST, 1);
	}
	// obtain stateNum, mixtureNum, vectorLen
	stateNum=0;
	mixtureNum=0;
	for(int i=0;i<M.modelNum;i++)
		stateNum+=(M.p2MMF[i]->stateNum-2); // discard 0 and n-1
	for(int i=0;i<M.tsNum;i++)
		mixtureNum+=M.ts[i].mixtureNum;
	// allocate memory
	HMMStateNum=new int[M.modelNum];
	HMMState=new int[stateNum];
	HMMStatePos=new int[M.modelNum];
	HMMStateMixtureNum=new int[M.tsNum];
	HMMMixturePos=new int[M.tsNum];
	HMMMeanVariancePos=new int[M.tsNum];
	HMMMean=new int[mixtureNum*vectorDim];
	HMMVariance=new int[mixtureNum*vectorDim];
	HMMMixtureWeight=new int[mixtureNum];
	HMMGconst=new int[mixtureNum];
	// HMMStateNum
	for(int i=0;i<M.modelNum;i++)
		HMMStateNum[i]=M.p2MMF[i]->stateNum;
	// HMMState
	c=0;
	for(int i=0;i<M.modelNum;i++){
		for(int j=0;j<M.p2MMF[i]->stateNum-2;j++){
			HMMState[c]=M.p2MMF[i]->tiedStateIndex[j];
			c++;
		}
	}
	// HMMStatePos
	c=0;
	HMMStatePos[0]=0;
	for(int i=1;i<M.modelNum;i++){
		c+=HMMStateNum[i-1]-2;
		HMMStatePos[i]=c;
	}
	// HMMstateMixtureNum
	for(int i=0;i<M.tsNum;i++)
		HMMStateMixtureNum[i]=M.ts[i].mixtureNum;
	// HMMMixturePos
	c=0;
	HMMMixturePos[0]=0;
	for(int i=1;i<M.tsNum;i++){
		c+=M.ts[i-1].mixtureNum;
		HMMMixturePos[i]=c;
	}
	// HMMMeanVariancePos
	c=0;
	HMMMeanVariancePos[0]=0;
	for(int i=1;i<M.tsNum;i++){
		c+=(M.ts[i-1].mixtureNum*vectorDim);
		HMMMeanVariancePos[i]=c;
	}
	// HMMMean, variance, gconst
	c=0;
	d=0;
	for(int i=0;i<M.tsNum;i++){
		for(int j=0;j<M.ts[i].mixtureNum;j++){
			for(int k=0;k<vectorDim;k++){
				HMMMean[c]=(int)floor(M.ts[i].mean[j*vectorDim+k]*IC_SCALE_MEAN+0.5);
				HMMVariance[c]=(int)floor(M.ts[i].variance[j*vectorDim+k]*IC_SCALE_VARIANCE+0.5);
				c++;
			}
			HMMMixtureWeight[d]=(int)floor(M.ts[i].mixtureWeight[j]*IC_SCALE_MIXWEIGHT+0.5);
			HMMGconst[d]=(int)floor(M.ts[i].gconst[j]*IC_SCALE_GCONST+0.5);
			d++;
		}
	}
}

void ICMACRO2HEADER::checkBoundary(float *dataVector, int vectorSize, int scale, int dataType){
	int v;
	if(dataType==0){ // short int:  -32,768 -> +32,767		
		for(int i=0;i<vectorSize;i++){
			v=(int)floor(dataVector[i]*scale+0.5);
			if((v>32767)||(v<-32768)){
				printf("%e with scale %d out of range\n", dataVector[i], scale);
				exit(1);
			}
		}
	}
	if(dataType==1){ // unsigned short int: 0 -> 65,535
		for(int i=0;i<vectorSize;i++){
			v=(int)floor(dataVector[i]*scale+0.5);
			if((v>65535)||(v<0)){
				printf("%e with scale %d out of range\n", dataVector[i], scale);
				exit(1);
			}
		}
	}
}

void ICMACRO2HEADER::doMacro2Header(char *inMacro, char *outFile){
	// read Macro
	M.readMacroBin(inMacro);

	UTILITY util;

	vectorDim=elimiateZeroValue();
	macro2vector();

	// output to header
	FILE *out=util.openFile(outFile, "wt");
	fprintf(out, "// ---------- MACRO ----------\n");
	fprintf(out, "const short int HMMNum=%d;\n", M.modelNum);
	fprintf(out, "const short int tsNum=%d;\n", M.tsNum);
	fprintf(out, "const short int streamWidth=%d;\n", vectorDim);
	// HMMName
	int HMMStrLen=0, tmpLen;
	for(int i=0;i<M.modelNum;i++){
		tmpLen=strlen(M.p2MMF[i]->str);
		if(tmpLen>HMMStrLen)
			HMMStrLen=strlen(M.p2MMF[i]->str);
	}
	fprintf(out, "const char HMMName[%d][%d]={", M.modelNum, HMMStrLen+1);
	char msg[MAXLEN], tmp[MAXLEN];
	strcpy(msg, "");
	for(int i=0;i<M.modelNum;i++){
		sprintf(tmp, "\"%s\",", M.p2MMF[i]->str);
		strcat(msg, tmp);
		if(i<M.modelNum-1){
			if((i!=0)&&(i%10==0)){
				fprintf(out, "%s\n", msg);
				strcpy(msg, "");
			}
		}
		else{
			msg[strlen(msg)-1]='\0';
			fprintf(out, "%s};\n\n", msg);
		}
	}
	// HMMStateNum
	fprintf(out, "const short int HMMStateNum[%d]={", M.modelNum);
	util.writeVector(out, HMMStateNum, M.modelNum, 50);
	// HMMState
	fprintf(out, "const short int HMMState[%d]={", stateNum);
	util.writeVector(out, HMMState, stateNum, 30);
	// HMMStatePos
	fprintf(out, "const short int HMMStatePos[%d]={", M.modelNum);
	util.writeVector(out, HMMStatePos, M.modelNum, 30);
	// HMMStateMixtureNum
	fprintf(out, "const short int HMMStateMixtureNum[%d]={", M.tsNum);
	util.writeVector(out, HMMStateMixtureNum, M.tsNum, 40);
	// HMMMixturePos
	fprintf(out, "const short int HMMMixturePos[%d]={", M.tsNum);
	util.writeVector(out, HMMMixturePos, M.tsNum, 40);
	// HMMMeanVariancePos
	fprintf(out, "const short int HMMMeanVariancePos[%d]={", M.tsNum);
	util.writeVector(out, HMMMeanVariancePos, M.tsNum, 40);
	// HMMMixtureWeight
	//fprintf(out, "const short int HMMMixtureWeight[%d]={", mixtureNum);
	//util.writeVector(out, HMMMixtureWeight, mixtureNum, 30);
	// HMMMean
	fprintf(out, "const short int HMMMean[%d]={", mixtureNum*vectorDim);
	util.writeVector(out, HMMMean, mixtureNum*vectorDim, 30);
	// HMMVariance
	fprintf(out, "const short int HMMVariance[%d]={", mixtureNum*vectorDim);
	util.writeVector(out, HMMVariance, mixtureNum*vectorDim, 50);
	// HMMGconst
	fprintf(out, "const unsigned short int HMMGconst[%d]={", mixtureNum);
	util.writeVector(out, HMMGconst, mixtureNum, 25);

	fclose(out);
}

int main(int argc, char **argv){
	// ==== For saving memory space, transition probs are no longer used. ====
	if(argc!=3){
		printf("Usage: %s DEST.macro macro.h\n", argv[0]);
		printf("Transition Probs are no longer used.\n");
		printf("Scale on mixWeight -> %d\n", IC_SCALE_MIXWEIGHT);
		printf("Scale on mean -> %d\n", IC_SCALE_MEAN);
		printf("Scale on (1/variance) -> %d\n", IC_SCALE_VARIANCE);
		printf("Scale on gconst -> %d\n", IC_SCALE_GCONST);
		exit(1);
	}
	
	ICMACRO2HEADER ICm2h;
	ICm2h.doMacro2Header(argv[1], argv[2]);

	return 0;
}
