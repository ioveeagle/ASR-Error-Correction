#include "recog-dnn.h"
#define DEBUG 0

/* Read HTK feature file */
void FEATURE::byteswap(void *ptr,int num) {
	char temp,*top_of_stack;
	int count=num/2;
	top_of_stack = (char*)ptr;
	for(int i=0;i<count;i++) {
		temp = *(top_of_stack+i);
		*(top_of_stack+i) = *(top_of_stack+num-1-i);
		*(top_of_stack+num-1-i) = temp;
	}
}

void FEATURE::FeaFieldInfo(char *feafilename) {
	size_t st;
	UTILITY util;

	FILE *feain=util.openFile(feafilename,"rb");
	int num_of_frames, frame_period;
	short int bytes_per_frame;
	st=fread(&num_of_frames,4,1,feain);     // 4 bytes
	byteswap(&num_of_frames,4);
	st=fread(&frame_period,4,1,feain);      // 4 bytes
	byteswap(&frame_period,4);
	st=fread(&bytes_per_frame,2,1,feain);   // 2 bytes
	byteswap(&bytes_per_frame,2);
	fclose(feain);

	frameNum=num_of_frames;
	frameWidth=bytes_per_frame/4;

	if((bytes_per_frame!=156)&&(bytes_per_frame!=104)){//!=39 and !=26
		printf("readfeaure.cpp: Error, streamWidth is %d\n", num_of_frames/4);
		exit(1);
	}
	if(frameNum>MAX_FRAME_NUM){
		printf("frameNum exceeds %d\n", MAX_FRAME_NUM);
		exit(1);
	}
}

// sizeBlock: # frames in a super-vector, i.e. 5
// vectorDim: dimension of feature vector, i.e. 39
// labelDelay:label delay in Kaldi LSTM decoder setting
void FEATURE::readMFCCforLSTM(char *feaFileName, int sizeBlock, int vectorDim, int labelDelay, int platform) {
	int cxtSize, cxtLeftSize, cxtRightSize;
	size_t st;
	UTILITY util;
	FILE *in;
	SCORETYPE *tmp=NULL;

	if( sizeBlock%2 != 1 ){
		printf("sizeBlock %d is not an odd number!\n", sizeBlock);
		exit(1);
	}
	cxtSize=(sizeBlock-1)>>1;
	if( labelDelay >= 0 ){
		if( labelDelay <= cxtSize )
			cxtLeftSize = cxtSize - labelDelay;
		else
			cxtLeftSize = 0;
		cxtRightSize = cxtSize + labelDelay;
	}else{
		printf("[ERROR] labelDelay should be >= 0, but you have %d\n", labelDelay);
		exit(1);
	}

	if(platform==0){
		FeaFieldInfo(feaFileName);
		tmp=new SCORETYPE[frameNum*frameWidth];
		// read file
		in=util.openFile(feaFileName,"rb");
		fseek(in, 12, SEEK_SET);
		st=fread(tmp, sizeof(SCORETYPE), frameNum*frameWidth, in);
		fclose(in);
		// do byte swap
		for(int i=0;i<frameNum;i++){
			for(int j=0;j<frameWidth;j++)
				byteswap(&tmp[i*frameWidth+j],4);
		}
		// copy to feaVec
		memset(feaVec, 0, sizeof(SCORETYPE)*MAX_FRAME_NUM*vectorDim);

		for(int i=0; i<cxtLeftSize; i++){ // left-context padding
			memcpy(feaVec+i*vectorDim, tmp, sizeof(SCORETYPE)*frameWidth);
		}

		for(int i=0; i<frameNum; i++){
			memcpy(feaVec+(i+cxtLeftSize)*vectorDim, tmp+i*frameWidth, sizeof(SCORETYPE)*frameWidth);
		}

		for(int i=0; i<cxtRightSize; i++){ // right-context padding
			memcpy(feaVec+(frameNum+cxtLeftSize+i)*vectorDim, tmp+(frameNum-1)*frameWidth, sizeof(SCORETYPE)*frameWidth);
		}


		if( DEBUG ){
			printf("frameNum:%d frameWidth:%d\n", frameNum, frameWidth);
			printf("cxtSize:%d cxtLeftSize:%d cxtRightSize:%d\n", cxtSize, cxtLeftSize, cxtRightSize);
			for(int i=0; i<cxtLeftSize+frameNum+cxtRightSize; i++){
				printf("\nFrame %d\n", i);
				for(int j=0; j<vectorDim; j++)
					printf("%f ", feaVec[i*vectorDim+j]);
			}		
		}
		// release memory
		if(tmp!=NULL){
			delete [] tmp;
			tmp=NULL;
		}
	}else{
		printf("[ERROR] current LSTM only support platform=0");
		exit(1);
	}
}
// PARAM platform 0: floating point, 1: fixed point
void FEATURE::readMFCCforDNN(char *feaFileName, int sizeBlock, int vectorDim, int platform) {
	size_t st;
	UTILITY util;
	FILE *in;
	SCORETYPE *tmp=NULL;

	if( sizeBlock%2 != 1 ){
		printf("sizeBlock %d is not an odd number!\n", sizeBlock);
		exit(1);
	}
	int cxtSize=(sizeBlock-1)>>1;

	if(platform==0){
		FeaFieldInfo(feaFileName);
		tmp=new SCORETYPE[frameNum*frameWidth];
		// read file
		in=util.openFile(feaFileName,"rb");
		fseek(in, 12, SEEK_SET);
		st=fread(tmp, sizeof(SCORETYPE), frameNum*frameWidth, in);
		fclose(in);
		// do byte swap
		for(int i=0;i<frameNum;i++){
			for(int j=0;j<frameWidth;j++)
				byteswap(&tmp[i*frameWidth+j],4);
		}
		// copy to feaVec
		memset(feaVec, 0, sizeof(SCORETYPE)*MAX_FRAME_NUM*vectorDim);

		for(int i=0; i<cxtSize; i++) // left-context padding
			memcpy(feaVec+i*vectorDim, tmp, sizeof(SCORETYPE)*frameWidth);

		for(int i=cxtSize;i<frameNum+cxtSize;i++)
			memcpy(feaVec+i*vectorDim, tmp+(i-cxtSize)*frameWidth, sizeof(SCORETYPE)*frameWidth);

		for(int i=0; i<cxtSize; i++) // right-context padding
			memcpy(feaVec+(frameNum+cxtSize+i)*vectorDim, tmp+(frameNum-1)*frameWidth, sizeof(SCORETYPE)*frameWidth);

		// release memory
		if(tmp!=NULL){
			delete [] tmp;
			tmp=NULL;
		}
	}
	else{
		// read feature vector		
		in=util.openFile(feaFileName, "rb");
		st=fread(&frameNum,sizeof(short int),1, in);
		short int *srcData=new short int[frameNum*26];
		st=fread(srcData, sizeof(short int), frameNum*26, in);
		fclose(in);
		// multiply by 4
		for(int i=0;i<frameNum*26;i++)
			srcData[i]=srcData[i]<<2;
		// copy to feaVec
		for(int i=0; i<cxtSize; i++) // left-context padding
			for(int j=0; j<26; j++)
				feaVec[i*28+j]=srcData[j];

		for(int i=cxtSize;i<frameNum+cxtSize;i++){
			for(int j=0;j<26;j++)
				feaVec[i*28+j]=srcData[(i-cxtSize)*26+j];
		}

		for(int i=0; i<cxtSize; i++) // right-context padding
			for(int j=0; j<26; j++)
				feaVec[(frameNum+cxtSize+i)*28+j]=srcData[frameNum*26+j];

		if(srcData!=NULL){
			delete [] srcData;
			srcData=NULL;
		}
	}
}


// PARAM platform 0: floating point, 1: fixed point
void FEATURE::readMFCC(char *feaFileName, int vectorDim, int platform) {
	size_t st;
	UTILITY util;
	FILE *in;
	SCORETYPE *tmp=NULL;

	if(platform==0){
		FeaFieldInfo(feaFileName);
		tmp=new SCORETYPE[frameNum*frameWidth];
		// read file
		in=util.openFile(feaFileName,"rb");
		fseek(in, 12, SEEK_SET);
		st=fread(tmp, sizeof(SCORETYPE), frameNum*frameWidth, in);
		fclose(in);
		// do byte swap
		for(int i=0;i<frameNum;i++){
			for(int j=0;j<frameWidth;j++)
				byteswap(&tmp[i*frameWidth+j],4);
		}
		// copy to feaVec
		memset(feaVec, 0, sizeof(SCORETYPE)*MAX_FRAME_NUM*vectorDim);
		for(int i=0;i<frameNum;i++)
			memcpy(feaVec+i*vectorDim, tmp+i*frameWidth, sizeof(SCORETYPE)*frameWidth);
		// release memory
		if(tmp!=NULL){
			delete [] tmp;
			tmp=NULL;
		}
	}
	else{
		// read feature vector		
		in=util.openFile(feaFileName, "rb");
		st=fread(&frameNum,sizeof(short int),1, in);
		short int *srcData=new short int[frameNum*26];
		st=fread(srcData, sizeof(short int), frameNum*26, in);
		fclose(in);
		// multiply by 4
		for(int i=0;i<frameNum*26;i++)
			srcData[i]=srcData[i]<<2;
		// copy to feaVec
		for(int i=0;i<frameNum;i++){
			for(int j=0;j<26;j++)
				feaVec[i*28+j]=srcData[i*26+j];
		}
		if(srcData!=NULL){
			delete [] srcData;
			srcData=NULL;
		}
	}
}

FEATURE::FEATURE(char *feaListFile, int vectorDim){
	// format: ref\t(spk_dir/fileName)
	// reference string is separated by space
	// ?? ?O ?e ?? ?? ?n ?? ?a ??	(900259_fea/025.mfc)
	UTILITY util;
	feaListNum=util.getFileLineCount(feaListFile);
	feaList = new FEALIST[feaListNum];

	FILE *in=util.openFile(feaListFile, "rt");
	char temp[MAXLEN], *pch, *rtn;

	for(int i=0;i<feaListNum;i++){
		rtn=fgets(temp, MAXLEN, in);
		temp[strlen(temp)-1]='\0'; // remove endofline

		if(temp[strlen(temp)-1]==')') // remove )
			temp[strlen(temp)-1]='\0';
		// transcription
		pch=strtok(temp, "\t");
		if(strlen(pch)>MAXLEN){
			printf("Line %d: ", i+1);
			printf("Reference length %d longer than MAX_HISTORY_LEN %d\n", (int)strlen(pch), MAX_HISTORY_LEN);
			exit(1);
		}
		strcpy(feaList[i].transcription, pch);
		// speaker
		pch=strtok(NULL, "(");
		strcpy(temp, pch);
		pch=strtok(temp, "_");
		strcpy(feaList[i].speaker, pch);
		// feaFile
		strcpy(feaList[i].feaFile, temp+strlen(pch)+1);		
	}
	fclose(in);

	// allocate feaVec
	feaVec = (SCORETYPE*) util.aligned_malloc(vectorDim*MAX_FRAME_NUM*sizeof(SCORETYPE), ALIGNSIZE);
}

FEATURE::~FEATURE(){
	UTILITY util;
	if(feaList!=NULL){
		delete [] feaList;
		feaList=NULL;
	}
	util.aligned_free(feaVec);
}
