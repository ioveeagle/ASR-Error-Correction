#include "recog.h"

typedef struct{
	char logModel[MODELSTRLEN];
	char phyModel[MODELSTRLEN];
} LOG2PHY;

class LOG2PHYMAPPING {
private:
	LOG2PHY *l2p;
	int log2phyNum;
	GRAPH G;
	void sortLog2Phy(LOG2PHY *m, int left, int right);
	int searchModel(LOG2PHY *m, int left,int right, char *inputstr);
	void mapping(char *p1In, char *p2In, char *p3In, char *phyStr);
	void readLog2Phy(char *log2phyFile);
public:
	LOG2PHYMAPPING();
	~LOG2PHYMAPPING();
	void doLog2Phy(char *graphLog, char *graphPhy, char *log2phyFile, char *symbolListFile);
};

LOG2PHYMAPPING::LOG2PHYMAPPING(){
	l2p=NULL;
}

LOG2PHYMAPPING::~LOG2PHYMAPPING(){
	if(l2p!=NULL){
		delete [] l2p;
		l2p=NULL;
	}
}

void LOG2PHYMAPPING::sortLog2Phy(LOG2PHY *m, int left, int right){
	int i=left, j=right;
	char key[MAXLEN];
	LOG2PHY temp[1];
	strcpy(key, m[(left+right)/2].logModel);
	//  partition
	do{
		while (strcmp(m[i].logModel, key)<0) i++;
		while (strcmp(m[j].logModel, key)>0) j--;
		if (i<=j){
			temp[0]=m[i]; m[i]=m[j]; m[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);

	//  recursion
	if (left<j) sortLog2Phy(m, left, j);
	if (i<right) sortLog2Phy(m, i, right);
}

int LOG2PHYMAPPING::searchModel(LOG2PHY *m, int left,int right,char *inputstr){
	int mid=(left+right)/2,difference;
	difference=strcmp(inputstr, m[mid].logModel);
	if (difference==0)
		return mid;
	(difference>0)?left=mid+1:right=mid-1;
	return ((left>right)||(right<left))?-1:searchModel(m,left,right,inputstr);
}

// mapping logical string to 'phyStr'
void LOG2PHYMAPPING::mapping(char *p1In, char *p2In, char *p3In, char *phyStr){
	int matchIndex;
	bool cond1, cond2;
	char str[MAXLEN], p1[100], p2[100], p3[100], outStr[MAXLEN];
	char strLC[MAXLEN], strRC[MAXLEN];

	str[0]='\0';

	if( (p1In != NULL) && (p2In == NULL) && (p3In == NULL) ){
		strcpy(str, p1In);
		if(strcmp(p1In, FSTR)==0)
			strcpy(str, EMPTY);
	}else{
		strcpy(p1, p1In);
		strcpy(p2, p2In);
		strcpy(p3, p3In);

		// step1: replace phones, <F> => <eps>
		if(strcmp(p1, FSTR)==0)
			strcpy(p1, EMPTY);
		if(strcmp(p2, FSTR)==0)
			strcpy(p2, EMPTY);
		if(strcmp(p3, FSTR)==0)
			strcpy(p3, EMPTY);
		
		// three context independent phone
		//    A-<eps>+B => <eps>
		//    A-sil+B => sil
		//    A-sp+B => sp
		if((strcmp(p2, EMPTY)==0)||(strcmp(p2, SIL)==0)||(strcmp(p2, "sp")==0)){
			// CD-phone
			strcpy(str, p2);
		}
	}

	// step2: check the phone in the left and the one in the right
	if(str[0]=='\0'){ // other cases, in other words, not <eps>, sil, sp
		cond1=(strcmp(p1, EMPTY)!=0); // is p1 NOT <eps>?
		cond2=(strcmp(p3, EMPTY)!=0); // is p3 NOT <eps>?
		if((cond1==true)&&(cond2==true)){// A-B+C
			strcpy(str, p1);
			strcat(str, "-");
			strcat(str, p2);
			strcat(str, "+");
			strcat(str, p3);

			strcpy(strLC, p1);
			strcat(strLC, "-");
			strcat(strLC, p2);

			strcpy(strRC, p2);
			strcat(strRC, "+");
			strcat(strRC, p3);
		}else if((cond1==true)&&(cond2==false)){
			//A-B+<eps> => A-B  old
			//A-B+<eps> => A-B+sil  since 20170928
			strcpy(str, p1);
			strcat(str, "-");
			strcat(str, p2);
			strcat(str, "+");
			strcat(str, "sil");
		}else if((cond1==false)&&(cond2==true)){
			//<eps>-A+B => A+B old
			//<eps>-A+B => sil-A+B since 20170928
			strcpy(str, "sil");
			strcat(str, "-");
			strcat(str, p2);
			strcat(str, "+");
			strcat(str, p3);
		}else{
			//<eps>-A+<eps> => A old
			//<eps>-A+<eps> => sil-A+sil since 20170928
			strcpy(str, "sil");
			strcat(str, "-");
			strcpy(str, p2);
			strcat(str, "+");
			strcat(str, "sil");
		}
	}

	// model mapping
	if(strcmp(str, EMPTY)!=0){ // NOT <eps>
		matchIndex=searchModel(l2p, 0, log2phyNum-1, str);

		// triphone not found, find left-context biphone instead
		if(matchIndex==-1){
			matchIndex=searchModel(l2p, 0, log2phyNum-1, strLC);
			if( matchIndex!=-1 )
				printf("\t%s back to %s\n", str, strLC);
		}

		// triphone not found, find right-context biphone instead
		if(matchIndex==-1){
			matchIndex=searchModel(l2p, 0, log2phyNum-1, strRC);
			if( matchIndex!=-1 )
				printf("\t%s back to %s\n", str, strRC);
		}

		if(matchIndex==-1){
			// ***NOTICE*** if a specific phone is not found, call out ASR team to produce one!
			printf("%s-%s+%s -> model %s not found\n", p1, p2, p3, str);
			strcpy(outStr, "!found_:(");
		}
		else
			strcpy(outStr, l2p[matchIndex].phyModel);
	}
	else
		strcpy(outStr, EMPTY);
	
	strcpy(phyStr, outStr);
}

void LOG2PHYMAPPING::readLog2Phy(char *fileName){
	UTILITY util;
	log2phyNum=util.getFileLineCount(fileName);
	l2p = new LOG2PHY[log2phyNum];
	
	char msg[MAXLEN], *src, *dst, *rtn;
	FILE *in=util.openFile(fileName, "rt");
	for(int i=0;i<log2phyNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		src=strtok(msg, "\t "); // logical model
		dst=strtok(NULL, "\t "); // physical model

		if(dst==NULL) // in ASUS project, this condition is always TRUE
			dst=src;

		if(strlen(src)>=MODELSTRLEN){// MODELSTRLEN = 30
			printf("%s exceeds MODELSTRLEN %d\n", src, MODELSTRLEN);
			exit(1);
		}
		if(strlen(dst)>=MODELSTRLEN){
			printf("%s exceeds MODELSTRLEN %d\n", dst, MODELSTRLEN);
			exit(1);
		}

		strcpy(l2p[i].logModel, src);
		strcpy(l2p[i].phyModel, dst);
	}
	fclose(in);

	sortLog2Phy(l2p, 0, log2phyNum-1); // sort according l2p[i].logModel
}

void LOG2PHYMAPPING::doLog2Phy(char *graphLog, char *graphPhy, char *log2phyFile, char *symbolListFile){
	G.readGraph(graphLog, symbolListFile, 0);
	readLog2Phy(log2phyFile);

	UTILITY util;
	FILE *out=util.openFile(graphPhy, "wt");
	char logMsg[MAXLEN], phyMsg[MAXLEN], *p1, *p2, *p3, temp[MAXLEN], msg[MAXLEN];
	for(int i=0;i<G.nodeNum;i++){
		for(int j=0;j<G.p2NO[i]->branchNum;j++){
			strcpy(logMsg, G.symbolList[G.p2NO[i]->arc[j].in].str); // string for logical model
			p1=strtok(logMsg, "-+"); // previous phone
			p2=strtok(NULL, "-+"); // current phone
			p3=strtok(NULL, "-+"); // next phone
			mapping(p1, p2, p3, phyMsg);

			sprintf(temp, "%d\t%d\t", i, G.p2NO[i]->arc[j].endNode); // path between two nodes
			strcpy(msg, temp);
			sprintf(temp, "%s\t%s", phyMsg, G.symbolList[G.p2NO[i]->arc[j].out].str); 
			strcat(msg, temp);
			if(G.p2NO[i]->arc[j].weight!=0){
				sprintf(temp, "\t%f", (float)log10((double)exp(G.p2NO[i]->arc[j].weight)));
				strcat(msg, temp);
			}
			fprintf(out, "%s\n", msg);
		}
	}
	for(int i=0;i<G.terminalNum;i++)
		fprintf(out, "%d\n", G.terminal[i]);
	fclose(out);
}

/** @brief  mapping logical labels to physical labels via log2phyList table
 *
 */
int main(int argc, char **argv){
	if(argc!=5){
		printf("Usage: %s graph.log graph.phy log2phyList sym\n", argv[0]);
		exit(1);
	}

	char graphLog[MAXLEN], graphPhy[MAXLEN], log2phyFile[MAXLEN], symbolListFile[MAXLEN];
	strcpy(graphLog, argv[1]); // input graph
	strcpy(graphPhy, argv[2]); // output graph
	strcpy(log2phyFile, argv[3]); // mapping table
	strcpy(symbolListFile, argv[4]); // CD phone list

	LOG2PHYMAPPING l2p;
	l2p.doLog2Phy(graphLog, graphPhy, log2phyFile, symbolListFile);

	return 0;
}
