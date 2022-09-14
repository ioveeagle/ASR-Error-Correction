#include "recog.h"

void GRAPH::allocateMem(char *graphFile){
	UTILITY util;	
	// nodeNum
	NO = new NODE_OFFLINE[nodeNum];
	p2NO = new NODE_OFFLINE * [nodeNum];
	// branchNum to allocate memory
	char msg[MAXLEN];	
	for(int i=0;i<nodeNum;i++)
		NO[i].branchNum=0;
	terminalNum=0;
	int rtn, s, e;
	FILE *in=util.openFile(graphFile, "rt");
	for(int i=0;i<graphFileLineNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		rtn=sscanf(msg,"%d %d ", &s, &e);
		// s: src index; e: dst index
		(rtn==2)?NO[s].branchNum++:terminalNum++;
	}
	fclose(in);
	// allocate memory
	for(int i=0;i<nodeNum;i++){
		NO[i].arc = new ARC[NO[i].branchNum];
		NO[i].branchNum=0;	// this will be updated in saveNNInfo
	}
	terminal=new int[terminalNum];
	terminalNum=0;	// this will be updated in saveNNInfo
}

// almost exactly the same as 'GRAPH::startEndInOutWeight()', except that input labels 'p3' have
// decoded from 'D=XX' to normal symbol index.
void GRAPH::saveNNInfoPartialExpand(char *str){
	UTILITY util;
	float weight;
	int branch, s, e, p;
	char msg[MAXLEN], *p1, *p2, *p3, *p4, *p5, inStr[MAXLEN], outStr[MAXLEN], *pch;
        bool symbolNotFound = false;

	strcpy(msg, str);
	p1=strtok(msg, "\t ");
	p2=strtok(NULL, "\t ");
	p3=strtok(NULL, "\t ");
	p4=strtok(NULL, "\t ");
	p5=strtok(NULL, "\t ");

	if(p2!=NULL){ // not terminal
		s=atoi(p1);
		e=atoi(p2);		
		branch=NO[s].branchNum; // NO[s].branchNum=0 in allocateMem()
		NO[s].arc[branch].endNode=e;

		strcpy(inStr, p3);
		if((inStr[0]=='D')&&(inStr[1]=='=')){
			pch=strtok(inStr, "=");
			pch=strtok(NULL, "=");
			NO[s].arc[branch].in=-atoi(pch)-DIC_OFFSET;	// make it negative
		}
		else{
			p=util.stringSearch(symbolList, 0, symbolNum-1, p3);
			if(p==-1){
				printf("%s, %s not found in symbolList\n", str, p3);
				exit(1);
			}
			NO[s].arc[branch].in=p;
		}

		(p4!=NULL)?strcpy(outStr, p4):strcpy(outStr, EMPTY);
		(p5!=NULL)?weight=(float)atof(p5):weight=0;

		p=util.stringSearch(symbolList, 0, symbolNum-1, p4);
		if(p==-1){
			printf("%s, %s not found in symbolList\n", str, p4);
			exit(1);
		}
		NO[s].arc[branch].out=p;
		NO[s].arc[branch].weight=(float)log((double)pow((float)10,weight));
		NO[s].branchNum++;
	}
	else{
		terminal[terminalNum]=atoi(p1);
		terminalNum++;
	}
}

// extract starting node index, ending node index, input label, output label, weight
void GRAPH::startEndInOutWeight(char *str){
	UTILITY util;
	int s, e, b, p;
	float weight;
	char msg[MAXLEN], *p1, *p2, *p3, *p4, *p5;

	strcpy(msg, str);
	p1=strtok(msg, "\t ");
	p2=strtok(NULL, "\t ");
	p3=strtok(NULL, "\t ");
	p4=strtok(NULL, "\t ");
	p5=strtok(NULL, "\t ");

	s=atoi(p1);
	if(p2!=NULL){
		// branchNum is obtained during allocateMem()
		// branchNum is # of branches out of node s
		b=NO[s].branchNum;	
		e=atoi(p2);

		NO[s].arc[b].endNode=e;

		p=util.stringSearch(symbolList, 0, symbolNum-1, p3);
		if(p==-1){
			printf("%s, %s not found in symbolList\n", str, p3);
			exit(1);
		}
		NO[s].arc[b].in=p;
		
		if(machineType==1){ // transducer
			p=util.stringSearch(symbolList, 0, symbolNum-1, p4);
			if(p==-1){
				printf("%s, %s not found in symbolList\n", str, p4);
				exit(1);
			}
			NO[s].arc[b].out=p;
			(p5==NULL)?weight=0:weight=(float)atof(p5);
			NO[s].arc[b].weight=(float)log((double)pow((float)10,weight));
		} 
		else{ // acceptor
			NO[s].arc[b].out=NO[s].arc[b].in;
			(p4==NULL)?weight=0:weight=(float)atof(p4);
			NO[s].arc[b].weight=(float)log((double)pow((float)10,weight));
		}
		NO[s].branchNum++;
	}
	else{
		terminal[terminalNum]=s;
		terminalNum++;
	}
}

void GRAPH::readGraph(char *graphFile, char *symListFile, int partialExpand){
	typedef struct {
		char str[200];
	} T;
	UTILITY util;
	// read symbolList
	symbolNum=util.getFileLineCount(symListFile);
	symbolList = new SYMBOLLIST[symbolNum];
	util.readSymbolList(symbolList, symListFile, symbolNum);
	// get graph info
	util.getGraphInfo(graphFile, graphFileLineNum, nodeNum, arcNum, machineType);
	// allocate memory
	allocateMem(graphFile);
	// read graph
	int segmentSize=100000, c;
	char msg[MAXLEN];
	T *t = new T[segmentSize];
	c=0;
	FILE *in=util.openFile(graphFile, "rt");
	for(int i=0;i<graphFileLineNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		strcpy(t[c].str, msg);
		c++;
		// the following codes are not executed until all the graph arcs are loaded or c==segmentSize
		if((c==segmentSize)||(i==graphFileLineNum-1)){
			for(int j=0;j<c;j++){
				if(partialExpand==0)
					startEndInOutWeight(t[j].str); // for each graph file line
				else
					saveNNInfoPartialExpand(t[j].str);
			}
			c=0;
		}		
	}
	fclose(in);
	// release memory
	if(t!=NULL){
		delete [] t;
		t=NULL;
	}

	for(int i=0;i<nodeNum;i++)
		p2NO[i]=NO+i; // copy NO to p2NO
}

int GRAPH::reDirectPathNum(char *inStr){
	int num=0, len=(int)strlen(inStr);
	for(int i=0;i<len;i++)
		if(inStr[i]=='.')
			num++;
	return num+1;
}

void GRAPH::readGraph2Decode(char *graphFile, char *symListFile, float alpha, int platform){
	size_t st;
	UTILITY util;
	FILE *in;

	int QScale;
	// determine platform and scale
	if((platform!=0)&&(platform!=1)){
		printf("Error while specifying platform: 0=floating point, 1=fixed point\n");
		exit(1);
	}
	(platform==0)?QScale=1:QScale=1024;
	// read symbol list
	in=util.openFile(symListFile, "rb");
	st=fread(&symbolNum, sizeof(int), 1, in);
	symbolList = new SYMBOLLIST[symbolNum];
	st=fread(symbolList, sizeof(SYMBOLLIST), symbolNum, in);
	//for(int i=0; i<symbolNum; i++)
	//	printf("%s\n", symbolList[i].str);
	fclose(in);
	// read graph
	in=util.openFile(graphFile, "rb");
	st=fread(&nodeNum, sizeof(int), 1, in);
	st=fread(&arcNum, sizeof(int), 1, in);
	st=fread(&terminalNum, sizeof(int), 1, in);

	// allocate memory
	N = new NODE[nodeNum];
	p2N = new NODE*[nodeNum]; // pointer to node
	arc=new ARC[arcNum];
	terminal=new int[terminalNum];
	unsigned short int *branchNumArray = new unsigned short int[nodeNum];
	// read data
	st=fread(branchNumArray, sizeof(unsigned short int), nodeNum, in);
	st=fread(arc, sizeof(ARC), arcNum, in);
	st=fread(terminal, sizeof(int), terminalNum, in);
	fclose(in);
	// update weight
	for(int i=0;i<arcNum;i++)
		arc[i].weight=arc[i].weight*QScale*alpha;
	// update arcPos
	for(int i=0;i<nodeNum;i++){
		N[i].branchNum=branchNumArray[i];
		p2N[i]=N+i;
		p2N[i]->feaFile=-1;
	}	
	p2N[0]->arcPos=0;
	for(int i=1;i<nodeNum;i++)
		p2N[i]->arcPos=p2N[i-1]->arcPos+p2N[i-1]->branchNum;

	delete [] branchNumArray;
}
/*
void GRAPH::readParallelGraph2Decode(char *graphList, int platform){
	size_t st;
	UTILITY util;
	FILE *in, *fin;
	char gFolder[MAXLEN], gTemp[MAXLEN];
	float gWeight[MAXLEN];
	int sN, nN, aN, tN; 
	int numGraph;
	int *accGraph;
	int idxGraph;
	int QScale;
	unsigned short int *branchNumArray;

	// determine platform and scale
	if((platform!=0)&&(platform!=1)){
		printf("Error while specifying platform: 0=floating point, 1=fixed point\n");
		exit(1);
	}
	(platform==0)?QScale=1:QScale=1024;

	nodeNum=0;
	arcNum=0;
	terminalNum=0;
	symbolNum=0;

	numGraph = util.getFileLineCount(garphList);	
	accGraph = new int[numGraph*4];
	idxGraph=0;

	in=util.openFile(graphList, "rt");
	while( fscanf(in, "%s %f", gFolder, &gWeight)==2 ){
		// read symbol list and retrieve the number
		strcpy( gTemp, gFolder);
		strcat( gTemp, "/DEST.sym");
		fin=util.openFile( gTemp, "rb");
		if( fin != NULL ){
			st=fread(&sN, sizeof(int), 1, fin);
			symbolNum+=sN;
			accGraph[numGraph*i]=symbolNum;
		}else{
			printf("[ERROR] cannot open symbol file %s\n", gTemp);
			exit(1);
		}
		fclose(fin);

		// read graph file and retrieve the nodeNum, arcNum, and terminalNum
		strcpy( gTemp, gFolder);
		strcat( gTemp, "/DEST.graph");
		fin=util.openFile(gTemp, "rb");
		if( fin != NULL ){
			st=fread(&nN, sizeof(int), 1, fin);
			st=fread(&aN, sizeof(int), 1, fin);
			st=fread(&tN, sizeof(int), 1, fin);
			nodeNum += nN;
			arcNum += aN;
			terminalNum += tN;

			accGraph[numGraph*idxGraph+1]=nodeNum;
			accGraph[numGraph*idxGraph+2]=arcNum;
			accGraph[numGraph*idxGraph+3]=terminalNum;
		}else{
			printf("[ERROR] cannot open graph file %s\n", gTemp);
                        exit(1);
		}
		fclose(fin);
		idxGraph++;
	}
	numGraph = idxGraph;

	// read in data
	fseek(in, 0, SEEK_SET);

	symbolList = new SYMBOLLIST[symbolNum];
	N = new NODE[nodeNum];
	p2N = new NODE*[nodeNum]; // pointer to node
	arc=new ARC[arcNum];
	terminal=new int[terminalNum];
	branchNumArray = new unsigned short int[nodeNum];

	idxGraph=0;
	while( fscanf(in, "%s %f", gFolder, &gWeight)==2 ){
		// read symbol list
		strcpy( gTemp, gFolder);
		strcat( gTemp, "/DEST.sym");
		fin=util.openFile( gTemp, "rb");
		if( fin != NULL ){
			st=fread(&sN, sizeof(int), 1, fin);
			st=fread(symbolList, sizeof(SYMBOLLIST), sN, in);
			symbolList+=sN;
		}else{
			printf("[ERROR] cannot open symbol file %s\n", gTemp);
			exit(1);
		}
		fclose(fin);

		// read graph file
		strcpy( gTemp, gFolder);
		strcat( gTemp, "/DEST.graph");
		fin=util.openFile(gTemp, "rb");
		if( fin != NULL ){
			st=fread(&nN, sizeof(int), 1, fin);
			st=fread(&aN, sizeof(int), 1, fin);
			st=fread(&tN, sizeof(int), 1, fin);

			st=fread(branchNumArray, sizeof(unsigned short int), nN, fin);
			st=fread(arc, sizeof(ARC), aN, fin);
			st=fread(terminal, sizeof(int), tN, fin);

			if( idxGraph>0 ){
				for(int i=0; i<aN; i++){
					arc[i].out += accGraph[numGraph*(idxGraph-1)]; // increase output symbol index on an arc
					arc[i].endNode += accGraph[numGraph*(idxGraph-1)+1]; // increase node index
				}

				for(int i=0; i<tN; i++){
					terminal[i] += accGraph[numGraph*(idxGraph-1)];
				}
			}

			branchNumArray+=nN;
			arc+=aN;
			terminal+=tN;
		}else{
			printf("[ERROR] cannot open graph file %s\n", gTemp);
                        exit(1);
		}
		fclose(fin);

		for(int i=cidx; i<aN; i++)
			arc[i].weight=arc[i].weight*QScale*alpha;

		idxGraph++;
	}
}
*/

GRAPH::GRAPH(){
	NO=NULL;
	p2NO=NULL;
	N=NULL;
	p2N=NULL;
	arc=NULL;
	terminal=NULL;
	symbolList=NULL;
}

GRAPH::~GRAPH(){
	if(NO!=NULL){
		delete [] NO;
		NO=NULL;
	}
	if(p2NO!=NULL){
		delete [] p2NO;
		p2NO=NULL;
	}
	if(N!=NULL){
		delete [] N;
		N=NULL;
	}
	if(p2N!=NULL){
		delete [] p2N;
		p2N=NULL;
	}
	if(arc!=NULL){
		delete [] arc;
		arc=NULL;
	}
	if(terminal!=NULL){
		delete [] terminal;
		terminal=NULL;
	}
	if(symbolList!=NULL){
		delete [] symbolList;
		symbolList=NULL;
	}
}
