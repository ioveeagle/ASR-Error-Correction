#include "recog.h"

UTILITY::UTILITY(){}

UTILITY::~UTILITY(){}

FILE * UTILITY::openFile(const char *fileName, const char *mode){
	FILE *fp;
	if ((fp = fopen(fileName, mode)) == NULL){
		printf("Unable to open %s\n",fileName);
		exit(1);
	}
	return(fp);
}

int UTILITY::getFileLineCount(char *fileName){
	int lineCount=0;
	FILE *in=openFile(fileName,"rt");
	char line[MAXLEN*320];
	while(fgets(line, MAXLEN*320, in)!=NULL)
		lineCount++;
	fclose(in);
	return lineCount;
}
// for recognition
void UTILITY::assignParameter(char *graphFile, char *macroBinFile, char *feaListFile, char *lookupTableFile, char *symbolListFile, char *dicFile, int &beamSize, float &alphaValue, int argc, char **argv){
	if(argc!=9){
		printf("Usage: %s graph macro.float feaList table sym dic beam alpha\n", argv[0]);
		exit(1);
	}

	strcpy(graphFile, argv[1]);
	strcpy(macroBinFile, argv[2]);
	strcpy(feaListFile, argv[3]);
	strcpy(lookupTableFile, argv[4]);
	strcpy(symbolListFile, argv[5]);
	strcpy(dicFile, argv[6]);
	beamSize=atoi(argv[7]);
	if(beamSize>300000){
		printf("beamSize is limited to 30000\n");
		exit(1);
	}
	alphaValue=(float)atof(argv[8]);
	if(alphaValue>1000){
		printf("alphaValue is limited to 1000\n");
		exit(1);
	}
}

int UTILITY::stringSearch(SYMBOLLIST *list, int left, int right, const char *inputstr){
	if(left<=right){
		int mid=(left+right)/2,difference;
		difference=strcmp(inputstr,list[mid].str);
		if (difference==0)
			return mid;
		(difference>0)?left=mid+1:right=mid-1;
	}
	return ((left>right)||(right<left))?-1:stringSearch(list, left, right, inputstr);
}

int UTILITY::stringSearch(MASTERMACROFILE **list, int left, int right, const char *inputstr){
	int mid=(left+right)/2,difference;
	difference=strcmp(inputstr,list[mid]->str);
	if (difference==0)
		return mid;
	(difference>0)?left=mid+1:right=mid-1;
	return ((left>right)||(right<left))?-1:stringSearch(list, left, right, inputstr);
}
#ifdef USE_LATTICE
void UTILITY::sortString(LATTICELIST *list, int low, int up){
	int i=low, j=up;
	char key[MAXLEN], temp[MAXLEN];
	strcpy(key, list[(low+up)/2].str);
	//  partition
	do{
		while (strcmp(list[i].str,key)<0) i++;
		while (strcmp(list[j].str,key)>0) j--;
		if (i<=j){
			strcpy(temp, list[i].str); strcpy(list[i].str, list[j].str); strcpy(list[j].str, temp);
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (low<j) sortString(list, low, j);
	if (i<up) sortString(list, i, up);
}

#endif 
void UTILITY::sortString(SYMBOLLIST *list, XINT low, XINT up){
	int i=low, j=up;
	char key[MAXLEN], temp[MAXLEN];
	strcpy(key, list[(low+up)/2].str);
	//  partition
	do{
		while (strcmp(list[i].str,key)<0) i++;
		while (strcmp(list[j].str,key)>0) j--;
		if (i<=j){
			strcpy(temp, list[i].str); strcpy(list[i].str, list[j].str); strcpy(list[j].str, temp);
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (low<j) sortString(list, low, j);
	if (i<up) sortString(list, i, up);
}


void UTILITY::representLink(LINK **k, NODE_OFFLINE **p, int nNum){
	// represent as link, lk must be allocated in advance
	int c=0;
	for(int i=0;i<nNum;i++){ // node
		for(int j=0;j<p[i]->branchNum;j++){ // branches from 'node'
			k[c]->start=i;
			k[c]->end=p[i]->arc[j].endNode;
			k[c]->in=p[i]->arc[j].in;
			k[c]->out=p[i]->arc[j].out;
			k[c]->weight=p[i]->arc[j].weight;
			c++;
		}
	}
}

void UTILITY::sortLink(LINK **k, int lkNum){
	// build map, pre-occupy pos according to k[]->start and occurrence
	typedef struct {
		int branchNum;
		int pos;
	} MAPINFO;
	int newNodeNum=maxNodeNumFromLink(k, lkNum);
	MAPINFO *ms=new MAPINFO[newNodeNum];
	for(int i=0;i<newNodeNum;i++)
		ms[i].branchNum=0;
	for(int i=0;i<lkNum;i++)
		ms[k[i]->start].branchNum++;
	ms[0].pos=0;
	for(int i=1;i<newNodeNum;i++)
		ms[i].pos=(ms[i-1].pos+ms[i-1].branchNum);
	// backup
	LINK **k2=new LINK*[lkNum];
	for(int i=0;i<lkNum;i++)
		k2[i]=k[i];
	// do sorting
	int p;
	for( int i=0;i<lkNum;i++){
		p=ms[k2[i]->start].pos;
		k[p]=k2[i];
		ms[k2[i]->start].pos++;
	}
	// release memory
	if(k2!=NULL){
		delete [] k2;
		k2=NULL;
	}
	if(ms!=NULL){
		delete [] ms;
		ms=NULL;
	}
}

// the first node should be indexed 0
void UTILITY::makeStartFromZero(LINK **k, int lkNum){
	int found=-1;
	//unsigned found=0;
	LINK *tmp=NULL;
	if(k[0]->start!=0){
		tmp=k[0];
		for(int i=1;i<lkNum;i++){
			if(k[i]->start==0){
				found=i;
				break;
			}
		}
		//if(found!=-1){
		if(found!=0){
			k[0]=k[found];
			k[found]=tmp;
		}
		else{
			printf("Node 0 not found\n");
			exit(1);
		}
	}
}

// maxNodeNumFromLink() returns maximal index from given links
int UTILITY::maxNodeNumFromLink(LINK **k, int lkNum){
	int maxNode=-1;
	for(int i=0;i<lkNum;i++){
		if(k[i]->start>maxNode)
			maxNode=k[i]->start;
		if(k[i]->end>maxNode)
			maxNode=k[i]->end;
	}
	return maxNode+1;
}

void UTILITY::nodeRenumerate(LINK **k, int lkNum, int *terminalNode, int terminalNodeNum){
	int nNum=maxNodeNumFromLink(k, lkNum);

	makeStartFromZero(k, lkNum);

	int *mapN=new int[nNum];
	memset(mapN, -1, sizeof(int)*nNum); // initialize all 'mapN' = -1
	for(int i=0;i<terminalNodeNum;i++) // terminal nodes = -2
		mapN[terminalNode[i]]=-2;
	// renumerate
	int c=0;
	for(int i=0;i<lkNum;i++){
		if(isTerminal(terminalNode, terminalNodeNum, k[i]->start)==false){
			if(mapN[k[i]->start]==-1){ // the node 'k[i]->start' hasn't be mapped
				mapN[k[i]->start]=c; // renumber node index
				c++;
			}
		}
		if(isTerminal(terminalNode, terminalNodeNum, k[i]->end)==false){
			if(mapN[k[i]->end]==-1){
				mapN[k[i]->end]=c;
				c++;
			}
		}
	}
	// reserve for terminals
	for(int i=0;i<nNum;i++){
		if(mapN[i]==-2){
			mapN[i]=c;
			c++;
		}
	}
	// reset node number
	for(int i=0;i<lkNum;i++){
		k[i]->start=mapN[k[i]->start];
		k[i]->end=mapN[k[i]->end];
	}
	for(int i=0;i<terminalNodeNum;i++)
		terminalNode[i]=mapN[terminalNode[i]];

	sortLink(k, lkNum);

	if(mapN!=NULL){
		delete [] mapN;
		mapN=NULL;
	}
}

void UTILITY::outputGraph(LINK **k, int lkNum, int *terminalNode, int terminalNodeNum, SYMBOLLIST *sym, int mType, char *outFile){
	FILE *out=openFile(outFile, "wt");
	char msg[MAXLEN];
	int s, e, inSym, outSym;
	float weight;
	for( int i=0;i<lkNum;i++){
		s=k[i]->start;
		e=k[i]->end;
		inSym=k[i]->in;
		outSym=k[i]->out;
		weight=k[i]->weight;
		outputTransition(s, e, inSym, outSym, weight, sym, mType, msg);
		fprintf(out, "%s\n", msg);
				
	}
	for(int i=0;i<terminalNodeNum;i++)
		fprintf(out, "%d\n", terminalNode[i]);
	fclose(out);
}

void UTILITY::outputGraph(NODE_OFFLINE **p, int nNum, int *terminalNode, int terminalNodeNum, SYMBOLLIST *sym, int mType, char *outFile){
	FILE *out=openFile(outFile, "wt");
	char msg[MAXLEN];
	int s, e, inSym, outSym;
	float weight;
	for(int i=0;i<nNum;i++){
		for(int j=0;j<p[i]->branchNum;j++){
			s=i;
			e=p[i]->arc[j].endNode;
			inSym=p[i]->arc[j].in;
			outSym=p[i]->arc[j].out;
			weight=p[i]->arc[j].weight;
			outputTransition(s, e, inSym, outSym, weight, sym, mType, msg);
			fprintf(out, "%s\n", msg);						
		}
	}
	for(int i=0;i<terminalNodeNum;i++)
		fprintf(out, "%d\n", terminalNode[i]);
	fclose(out);
}

void UTILITY::outputTransition(int s, int e, int in, int out, float weight, SYMBOLLIST *sl, int mType, char *outStr){
	char msg[MAXLEN], temp[MAXLEN];
	
	float outWeight = (float)log10((double)exp(weight)); // Added by Chi-Yueh Lin @20110804
	if( outWeight < -DBL_MAX ){ outWeight=-99.0; } // Added by Chi-Yueh Lin @20110804
	
	if(mType==0) // acceptor
		sprintf(temp, "%d\t%d\t%s", s, e, sl[in].str);
	else // transducer
		sprintf(temp, "%d\t%d\t%s\t%s", s, e, sl[in].str, sl[out].str);
	strcpy(msg, temp);
	if(weight!=0){
		//sprintf(temp, "\t%f", (float)log10((double)exp(weight)));
		sprintf(temp, "\t%f", outWeight); // Added by Chi-Yueh Lin @20110804
		strcat(msg, temp);
	}
	strcpy(outStr, msg);
	
	//if( (float)log10((double)exp(weight)) < -DBL_MAX ) printf("overflow ");
	//printf("%d %d %f %f %f %s \n", in, out, weight, exp(weight), (float)log10((double)exp(weight)), temp);
}

bool UTILITY::isNumeric(const char *inStr){
    if( (inStr == NULL)||(*inStr=='\0'))
		return false;
	char *p;
	double d;
	d=strtod (inStr, &p);
	return *p == '\0';
}

bool UTILITY::isTerminal(int *terminalNode, int terminalNodeNum, int node){
	int found=-1;
	bool result;
	for(int i=0;i<terminalNodeNum;i++){
		if(terminalNode[i]==node){
			found=i;
			break;
		}
	}
	(found!=-1)?result=true:result=false;
	return result;
}

void UTILITY::readSymbolList(SYMBOLLIST *sym, char *fileName, int symNum){
	char msg[MAXLEN], *pch, *rtn;
	FILE *in=openFile(fileName, "rt");
	for(int i=0;i<symNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		pch=strtok(msg, "\t "); // delimeter could be 'tab' or 'space'
		if(strlen(pch)+1<WORDLEN) // WORDLEN=50
			strcpy(sym[i].str, pch);
		else{
			printf("Length of %s exceeds WORDLEN=%d\n", msg, WORDLEN);
			exit(1);
		}
	}
	fclose(in);
	sortString(sym, 0, symNum-1);
}

#ifdef USE_LATTICE
int UTILITY::uniqueSymbol(LATTICELIST *sl, int slNum){
	char cmp[MAXLEN];
	int c=0;
	if(slNum>0){
		sortString(sl, 0, slNum-1);

		LATTICELIST *tmp=new LATTICELIST[slNum];

		strcpy(cmp, sl[0].str);
		strcpy(tmp[0].str, sl[0].str);
		c=1;
		for(int i=1;i<slNum;i++){
			if(strcmp(sl[i].str, cmp)!=0){
				strcpy(cmp, sl[i].str);
				strcpy(tmp[c].str, sl[i].str);
				c++;
			}
		}

		memcpy(sl, tmp, sizeof(LATTICELIST)*c);

		delete [] tmp;
		tmp=NULL;
	}
	return c;
}
#endif

int UTILITY::uniqueSymbol(SYMBOLLIST *sl, int slNum){
	char cmp[MAXLEN];
	int c=0;
	if(slNum>0){
		sortString(sl, 0, slNum-1);

		SYMBOLLIST *tmp=new SYMBOLLIST[slNum];

		strcpy(cmp, sl[0].str);
		strcpy(tmp[0].str, sl[0].str);
		c=1;
		for(int i=1;i<slNum;i++){
			if(strcmp(sl[i].str, cmp)!=0){
				strcpy(cmp, sl[i].str);
				strcpy(tmp[c].str, sl[i].str);
				c++;
			}
		}

		memcpy(sl, tmp, sizeof(SYMBOLLIST)*c);

		delete [] tmp;
		tmp=NULL;
	}
	return c;
}

void UTILITY::sortDic(DIC *d, int left, int right){
	int i=left, j=right;
	int key;
	DIC temp[1];
	key=d[(left+right)/2].word;
	//  partition
	do{
		while (d[i].word-key<0) i++;
		while (d[j].word-key>0) j--;
		if (i<=j){
			temp[0]=d[i]; d[i]=d[j]; d[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (left<j) sortDic(d, left, j);
	if (i<right) sortDic(d, i, right);
}

void UTILITY::readDic(DIC *d, int &entryNum, SYMBOLLIST *sl, int slNum, char *inDic){
	FILE *in=openFile(inDic, "rt");
	char msg[MAXLEN], chkstr[MAXLEN], *pch, *rtn, *p1, *p2;
	int j, p;
	for(int i=0;i<entryNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';

		strcpy(chkstr, msg);
		p1=strtok(chkstr, "\t ");
		p2=strtok(NULL, "\t ");
		if((p1==NULL)||(p2==NULL)){
			printf("Dic entry error: line %d, %s\n", i+1, msg);
			exit(1);
		}

		pch=strtok(msg, "\t ");
		// word
		p=stringSearch(sl, 0, slNum-1, pch);
		if(p==-1){
			printf("%s not found in symbolList\n", pch);
			exit(1);
		}
		d[i].word=p;
		j=0;
		// phone
		memset(d[i].phone, -1, sizeof(int)*MAX_PHONE_NUM);
		pch=strtok(NULL, "\t ");
		while(pch!=NULL){
			p=stringSearch(sl, 0, slNum-1, pch);
			if(p==-1){
				printf("%s not found in symbolList\n", pch);
				exit(1);
			}
			d[i].phone[j]=p;
			j++;
			pch=strtok(NULL, "\t ");
		}
		d[i].phoneNum=j;
	}
	fclose(in);

	sortDic(d, 0, entryNum-1);
	
	// remove duplicated entries
	int cmp, c=0;
	for(int i=0;i<entryNum-1;i++){
		if(d[i].word==d[i+1].word){
			cmp=memcmp(d[i].phone, d[i+1].phone, sizeof(int)*MAX_PHONE_NUM);
			if(cmp==0)// discard entry i+1
				d[i+1].word=-1;
		}
	}
	for(int i=0;i<entryNum;i++){
		if(d[i].word!=-1)
			c++;
	}
	DIC *uniqueD = new DIC[c];

	c=0;
	for(int i=0;i<entryNum;i++){
		if(d[i].word!=-1){
			uniqueD[c]=d[i];
			c++;
		}
	}
	sortDic(uniqueD, 0, c-1);
	for(int i=0;i<c;i++)
		d[i]=uniqueD[i];
	entryNum=c;

	delete [] uniqueD;
}

void UTILITY::getGraphInfo(char *inFile, int &inFileLineNum, int &nodeNum, int &arcNum, int &machineType){
	int segmentSize=3000000, num;	
	int c;
	char *p1=NULL, *p2=NULL, *p3=NULL, *p4=NULL, msg[MAXLEN], *rtn=NULL;
	inFileLineNum=getFileLineCount(inFile);

	typedef struct {
		char str[100];
	}T;
	T *t=new T[segmentSize]; //up to 3M lines

	arcNum=0;
	nodeNum=0;
	c=0;
	machineType=0; // 0: acceptor, 1: transducer

	FILE *in=openFile(inFile, "rt");
	for(int i=0;i<inFileLineNum;i++){
		// read part of it into memory
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		strcpy(t[c].str, msg);
		c++;
		// the following codes are not executed until the entire graph file is read into RAM
		if((c==segmentSize)||(i==inFileLineNum-1)){
			for(int j=0;j<c;j++){
				p1=strtok(t[j].str, "\t "); // src index
				p2=strtok(NULL, "\t "); // dst index
				p3=strtok(NULL, "\t "); // input label
				p4=strtok(NULL, "\t "); // output label
				if(p1!=NULL){
					num=atoi(p1);
					if(num>nodeNum) 
						nodeNum=num; // total # of nodes counter: nodeNum = max[p1, nodeNum]
				}
				if(p2!=NULL){
					num=atoi(p2);
					if(num>nodeNum) // nodeNum = max[p1, nodeNum]
						nodeNum=num;
					arcNum++; // valid (p1,p2) pair denotes a complete arc, so accumulate it!
					// check machine type
					if(machineType==0){
						if(p4!=NULL){ // it could either be a number (transducer) or a string (acceptor)
							if(isNumeric(p4)==false) // a label, for acceptor
								machineType=1;
						}
					}
				}
			}
			c=0;
		}
	}
	fclose(in);
	nodeNum++; // since 'nodeNum' starts from 0, we have to add additional one state
	// release memory
	if(t!=NULL){
		delete [] t;
		t=NULL;
	}
}

void UTILITY::writeVector(FILE *f, int *s, int num, int breakSize){
	int cnt=0;
	char msg[MAXLEN], tmp[MAXLEN];
	strcpy(msg, "");
	while(cnt<num){
		sprintf(tmp, "%d,", s[cnt]);
		strcat(msg, tmp);
		cnt++;
		if(cnt%breakSize==0){
			if(cnt==num){
				msg[strlen(msg)-1]='\0';
				fprintf(f, "%s", msg);
			}
			else
				fprintf(f, "%s\n", msg);				
			strcpy(msg, "");
		}
	}
	if(strlen(msg)!=0){
		msg[strlen(msg)-1]='\0';
		fprintf(f, "%s};\n\n", msg);
	}
	else
		fprintf(f, "};\n\n");
}

void UTILITY::quickSort(ARENA **p2arena,int low,int up) {   // Sort arena based on each element's score
	int i,j;SCORETYPE pkey;
	ARENA *temp;
	if(low<up) {
		i=low;j=up+1;
		pkey=p2arena[low]->score;
		do {
			do {i++;} while((p2arena[i]->score>=pkey)&&(i<up));
			do {j--;} while((p2arena[j]->score<pkey)&&(j>low));
			if (i<j) {
				temp=p2arena[i];
				p2arena[i]=p2arena[j];
				p2arena[j]=temp;
			}
		} while(i<j);
		temp=p2arena[j]; p2arena[j]=p2arena[low]; p2arena[low]=temp;
		quickSort(p2arena,low,j-1);
		quickSort(p2arena,j+1,up);
	}
}

int UTILITY::partition(ARENA **p2arena, int deb, int fin) {
	ARENA *temp;
	int compt, pivotIdx, i;
	pivotIdx = compt = deb;
	SCORETYPE score=p2arena[pivotIdx]->score;
	for(i=deb+1;i<=fin;i++){
		if (p2arena[i]->score>score){
			compt++;
			temp=p2arena[compt]; p2arena[compt]=p2arena[i]; p2arena[i]=temp; // swap
		}
	}
	temp=p2arena[compt]; p2arena[compt]=p2arena[deb]; p2arena[deb]=temp; // swap
	return compt;
}

void UTILITY::pseudoSort(ARENA **p2arena, int low, int up, int topN){
	ARENA *temp;
	int splitPosition, pkey;
	if(low<up){
		pkey=(low+up)/2;
		temp=p2arena[low]; p2arena[low]=p2arena[pkey]; p2arena[pkey]=temp; // swap
		splitPosition=partition(p2arena, low, up);
		if(splitPosition+1>topN)
			pseudoSort(p2arena, low, splitPosition, topN);
		if(splitPosition<topN-1)
			pseudoSort(p2arena, splitPosition+1, up, topN);
	}
}

#ifdef USE_LATTICE
void UTILITY::showLatticeMem(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum, int frame, int type)
{
	int pos, pos2, slNum=0;
	char outWord[WORDLEN], outWord2[WORDLEN], inWord[WORDLEN]; 
	bool c1, c2, c3, c4;
	float lmscore, amscore;
	char info[6][WORDLEN];
	char tmp[MAXLEN], tmp2[MAXLEN];

	if( frame==0 ){
		latLine = 0;
		strPos = 0;
		memLattice = (char*)malloc(sizeof(char)*LATTICE_LINE*LATTICE_LINE_LEN);		
	}

	if( type > 0 ){
		LATTICELIST *sl = new LATTICELIST[showNum];
		for(int i=0;i<showNum;i++){
			int j=h[p2a[i]->hist].pathNum-1;

			if( (h[p2a[i]->hist].frame == frame) && (j>=0) ){
				pos=h[p2a[i]->hist].arc[j];
				(j>0)?pos2=h[p2a[i]->hist].arc[j-1]:pos2=-1;

				strcpy(outWord, g.symbolList[g.arc[pos].out].str);
				(pos2==-1)?strcpy(outWord2, "<eps>"):strcpy(outWord2, g.symbolList[g.arc[pos2].out].str);


				(strcmp(outWord, SIL)==0)?c1=true:c1=false;
				(strcmp(outWord, EMPTY)==0)?c2=true:c2=false;
				(strcmp(outWord, "</s>")==0)?c3=true:c3=false;

				// sil, BUT we ignore the SENT-START silence because its boundaries and scores are not accurate.
				if((c1==true) && (j>0) ){ 
					amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-1];
					lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-1];
					sprintf(sl[slNum].str, "%04d %04d %s %s %f %f", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
					slNum++;
				}

				// discard sil, <eps> and </s>
				if((c1==false)&&(c2==false)&&(c3==false)){
					if( j>0 ){
						amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-1];
						lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-1];
						sprintf(sl[slNum].str, "%04d %04d %s %s %f %f", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
					}else{
						amscore = h[p2a[i]->hist].amScore[j];
						lmscore = h[p2a[i]->hist].lmScore[j];
						sprintf(sl[slNum].str, "%04d %04d %s %s %f %f", 0, h[p2a[i]->hist].frameBoundary[j], outWord, outWord2, amscore, lmscore);
					}

					slNum++;	
				}

				// SENT-END silence
				if(c3==true){ 
					strcpy(outWord, "sil");
					amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-1];
					lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-1];
					sprintf(sl[slNum].str, "%04d %04d %s %s %f %f", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
					slNum++;
				} 
			}
		}
	
		int usl=0;

		if(slNum>0)
			usl = uniqueSymbol(sl, slNum);

		if(usl>0){
			//memcpy(memLattice+latLine*LATTICE_LINE_LEN, sl[0].str, strlen(sl[0].str)+1);
			strcpy(memLattice+strPos, sl[0].str);
			strPos += strlen(sl[0].str)+1;
			latLine++;
									
			sscanf(sl[0].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
			sprintf(tmp, "%s %s %s %s", info[0], info[1], info[2], info[3]);
			
			for(int i=1; i<usl; i++){
				
				sscanf(sl[i].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
				sprintf(tmp2, "%s %s %s %s", info[0], info[1], info[2], info[3]);
				
				if( strcmp(tmp, tmp2)!=0 ){				
					strcpy(memLattice+strPos, sl[i].str);
					strPos+=strlen(sl[i].str)+1;
					latLine++;
										
					strcpy(tmp, tmp2);
				}
			}
			
			delete [] sl;
			sl = NULL;			
		}
		
	}else{
		int lineNum = latLine;
		int curPos = 0;

		LATTICELIST *sl = new LATTICELIST[lineNum];

		strcpy(sl[0].str, memLattice);		
		strPos = strlen(sl[0].str)+1;
		for(int i=1; i<lineNum; i++){
			strcpy(sl[i].str, memLattice+strPos);
			strPos+=strlen(sl[i].str)+1;
		}

		int usl = uniqueSymbol(sl, lineNum);
		FILE *out = fopen(fileName, "wb");
			
		memset(memLattice, 0, lineNum*LATTICE_LINE_LEN);

		//memcpy(memLattice, sl[0].str, strlen(sl[0].str));
		strcpy(memLattice, sl[0].str);		
		curPos = strlen(sl[0].str);
		memcpy(memLattice+curPos, "\n", 1);
		curPos+=1;
				
		sscanf(sl[0].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
		sprintf(tmp, "%s %s %s %s", info[0], info[1], info[2], info[3]);
		
		for(int i=1; i<usl; i++){

			sscanf(sl[i].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
			sprintf(tmp2, "%s %s %s %s", info[0], info[1], info[2], info[3]);

			if( strcmp(tmp, tmp2)!=0 ){
				strcpy(memLattice+curPos, sl[i].str);
				curPos += strlen(sl[i].str);
				memcpy(memLattice+curPos, "\n", 1);
				curPos+=1;

				strcpy(tmp, tmp2);
			} 

		}
		
		fwrite(memLattice, 1, curPos, out);
		fclose(out);

		free(memLattice);
		delete [] sl;
		sl = NULL;		

	}

}

void UTILITY::showLattice(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum, int frame, int type){
	int pos, pos2, slNum=0;
	char outWord[WORDLEN], outWord2[WORDLEN], inWord[WORDLEN]; 
	bool c1, c2, c3, c4;
	float lmscore, amscore, alscore;
	char info[6][WORDLEN];

	if( type > 0 ){
		LATTICELIST *sl = new LATTICELIST[showNum];
		FILE *out=openFile(fileName, "at+");
		for(int i=0;i<showNum;i++){
			int j=h[p2a[i]->hist].pathNum-1;

			//if( (h[p2a[i]->hist].frame == frame) && (j>=0) ){
			if( (h[p2a[i]->hist].frameBoundary[j] == frame) && (j>=0) ){
				pos=h[p2a[i]->hist].arc[j];
				(j>0)?pos2=h[p2a[i]->hist].arc[j-1]:pos2=-1;

				strcpy(outWord, g.symbolList[g.arc[pos].out].str);
				(pos2==-1)?strcpy(outWord2, "<eps>"):strcpy(outWord2, g.symbolList[g.arc[pos2].out].str);


				(strcmp(outWord, SIL)==0)?c1=true:c1=false;
				(strcmp(outWord, EMPTY)==0)?c2=true:c2=false;
				(strcmp(outWord, "</s>")==0)?c3=true:c3=false;

				//(h[p2a[i]->hist].frameBoundary[j]!=frame)?c4=true:c4=false;
				(h[p2a[i]->hist].frameBoundary[j]<frame-1)?c4=true:c4=false;

				//printf("fr:%d frH:%d fB:%d j:%d W:%s\n", frame, h[p2a[i]->hist].frame, h[p2a[i]->hist].frameBoundary[j], j, outWord);
				//c1=false; // forced to output 'sil'
				
				// sil, BUT we ignore the SENT-START silence because its boundaries and scores are not accurate.
				if((c1==true) && (j>0) && (c4==false)){ 
					amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-1];
					lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-1];
					#ifdef LATTICE_SIMPLE
					alscore = amscore+lmscore;
					if(atoi(outWord+1)==0) strcpy(outWord, "s0");
					sprintf(sl[slNum].str, "%04d %04d %s %f\n", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord+1, alscore);
					#else
					sprintf(sl[slNum].str, "%04d %04d %s %s %f %f\n", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
					#endif

					//printf("%d %s", frame, sl[slNum].str);
					slNum++;
				}

				// discard sil, <eps> and </s>
				if((c1==false)&&(c2==false)&&(c3==false)&& (c4==false)){
					if( j>0 ){
						amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-1];
						lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-1];

						#ifdef LATTICE_SIMPLE	
						alscore = amscore+lmscore;
						if(atoi(outWord+1)==0) strcpy(outWord, "s0");
						sprintf(sl[slNum].str, "%04d %04d %s %f\n", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord+1, alscore);
						#else
						sprintf(sl[slNum].str, "%04d %04d %s %s %f %f\n", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
						#endif
					}else{
						amscore = h[p2a[i]->hist].amScore[j];
						lmscore = h[p2a[i]->hist].lmScore[j];

						#ifdef LATTICE_SIMPLE	
						alscore = amscore+lmscore;
						if(atoi(outWord+1)==0) strcpy(outWord, "s0");
						sprintf(sl[slNum].str, "%04d %04d %s %f\n", 0, h[p2a[i]->hist].frameBoundary[j], outWord+1, alscore);
						#else
						sprintf(sl[slNum].str, "%04d %04d %s %s %f %f\n", 0, h[p2a[i]->hist].frameBoundary[j], outWord, outWord2, amscore, lmscore);
						#endif						
					}
					//printf("%d %s", frame, sl[slNum].str);
					slNum++;	
				}

				// SENT-END silence
				if(c3==true && (c4==false)){ 
					strcpy(outWord, "sil");
					amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-1];
					lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-1];

					#ifdef LATTICE_SIMPLE	
					alscore = amscore+lmscore;
					if(atoi(outWord+1)==0) strcpy(outWord, "s0");
					sprintf(sl[slNum].str, "%04d %04d %s %f\n", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord+1, alscore);
					#else
					sprintf(sl[slNum].str, "%04d %04d %s %s %f %f\n", h[p2a[i]->hist].frameBoundary[j-1], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
					#endif
					//printf("%d %s", frame, sl[slNum].str);

					slNum++;
				} 
				

			}
		}

		int usl=0;

		#ifdef LATTICE_SIMPLE
		for(int i=0; i<slNum; i++)
			fprintf(out, "%s", sl[i].str);
		#else
		if(slNum>0){
			usl = uniqueSymbol(sl, slNum);
		}

		char tmp[MAXLEN], tmp2[MAXLEN];

		if( (usl>0) ){
			
			fprintf(out, "%s", sl[0].str);
	
			sscanf(sl[0].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
			sprintf(tmp, "%s %s %s %s", info[0], info[1], info[2], info[3]);
			
			for(int i=1; i<usl; i++){
				sscanf(sl[i].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
				sprintf(tmp2, "%s %s %s %s", info[0], info[1], info[2], info[3]);
				
				if( strcmp(tmp, tmp2)!=0 ){
					fprintf(out, "%s", sl[i].str);
					strcpy(tmp, tmp2);
				}
			}
			
			delete [] sl;
			sl = NULL;		
		}
		#endif

		fclose(out);
		
	}else{
	
		#ifndef LATTICE_SIMPLE	
		int lineNum = getFileLineCount(fileName);		

		LATTICELIST *sl = new LATTICELIST[lineNum];
		FILE *in=openFile(fileName, "rt");

		for(int i=0; i<lineNum; i++)
			fgets( sl[i].str, MAXLEN, in);
		fclose(in);

		FILE *out=openFile(fileName, "wt");
		int usl = uniqueSymbol(sl, lineNum);
		char tmp[MAXLEN], tmp2[MAXLEN];
		
			
		fprintf(out, "%s", sl[0].str);
		sscanf(sl[0].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
		sprintf(tmp, "%s %s %s %s", info[0], info[1], info[2], info[3]);

		for(int i=1; i<usl; i++){
			sscanf(sl[i].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
			sprintf(tmp2, "%s %s %s %s", info[0], info[1], info[2], info[3]);
			
			if( strrchr(tmp2, ' ')!=NULL ){
			  if( strcmp(tmp, tmp2)!=0 ){
				fprintf(out, "%s", sl[i].str);
				strcpy(tmp, tmp2);
			  } 
                        }
		}
		

		fclose(out);

		delete [] sl;
		sl = NULL;	
		#endif	
	}

}

void UTILITY::showLattice2(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum, int frame, int type){
	int pos, pos2, slNum=0;
	char outWord[WORDLEN], outWord2[WORDLEN], inWord[WORDLEN]; 
	bool c1, c2, c3, c4;
	float lmscore, amscore, alscore;
	char info[6][WORDLEN];

	if( type > 0 ){
		LATTICELIST *sl = new LATTICELIST[showNum];
		FILE *out=openFile(fileName, "at+");
		for(int i=0;i<showNum;i++){
			int j=h[p2a[i]->hist].pathNum-1;
			strcpy(inWord, g.symbolList[g.arc[h[p2a[i]->hist].arc[j]].in].str);

			//if( (h[p2a[i]->hist].frame == frame) && (j>=0) ){
			if( (h[p2a[i]->hist].frameBoundary[j] == frame) && (j>=0) && ((strcmp(inWord, "<eps>")==0)||(strcmp(inWord,"</s>")==0) ) ){
				(j>=1)?pos=h[p2a[i]->hist].arc[j-1]:pos=-1;
				(j>=3)?pos2=h[p2a[i]->hist].arc[j-3]:pos2=-1;

				(pos==-1)?strcpy(outWord, "<eps>"):strcpy(outWord, g.symbolList[g.arc[pos].out].str);
				(pos2==-1)?strcpy(outWord2, "<eps>"):strcpy(outWord2, g.symbolList[g.arc[pos2].out].str);
				
				/*
				(strcmp(outWord, SIL)==0)?c1=true:c1=false;
				(strcmp(outWord, EMPTY)==0)?c2=true:c2=false;
				(strcmp(outWord, "</s>")==0)?c3=true:c3=false;
				*/
				if( pos2==-1 ) strcpy(outWord2, "sil");
				if(strcmp(outWord, "</s>")==0) strcpy(outWord, "sil");
				
				if( j>= 2 ){
					amscore = h[p2a[i]->hist].amScore[j]-h[p2a[i]->hist].amScore[j-2];
					lmscore = h[p2a[i]->hist].lmScore[j]-h[p2a[i]->hist].lmScore[j-2];
					sprintf(sl[slNum].str, "%04d %04d %s %s %f %f\n", h[p2a[i]->hist].frameBoundary[j-2], h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);

				}else{
					amscore = h[p2a[i]->hist].amScore[j];
					lmscore = h[p2a[i]->hist].lmScore[j];
					sprintf(sl[slNum].str, "%04d %04d %s %s %f %f\n", 0, h[p2a[i]->hist].frameBoundary[j]-1, outWord, outWord2, amscore, lmscore);
				}
					
				slNum++;
			}
		}

		int usl=0;

		#ifdef LATTICE_SIMPLE
		for(int i=0; i<slNum; i++)
			fprintf(out, "%s", sl[i].str);
		#else
		if(slNum>0){
			usl = uniqueSymbol(sl, slNum);
		}

		char tmp[MAXLEN], tmp2[MAXLEN];

		if( (usl>0) ){
			
			fprintf(out, "%s", sl[0].str);
	
			sscanf(sl[0].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
			sprintf(tmp, "%s %s %s %s", info[0], info[1], info[2], info[3]);
			
			for(int i=1; i<usl; i++){
				sscanf(sl[i].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
				sprintf(tmp2, "%s %s %s %s", info[0], info[1], info[2], info[3]);
				
				if( strcmp(tmp, tmp2)!=0 ){
					fprintf(out, "%s", sl[i].str);
					strcpy(tmp, tmp2);
				}
			}
			
			delete [] sl;
			sl = NULL;		
		}
		#endif

		fclose(out);
		
	}else{
	
		#ifndef LATTICE_SIMPLE	
		int lineNum = getFileLineCount(fileName);		

		LATTICELIST *sl = new LATTICELIST[lineNum];
		FILE *in=openFile(fileName, "rt");

		for(int i=0; i<lineNum; i++)
			fgets( sl[i].str, MAXLEN, in);
		fclose(in);

		FILE *out=openFile(fileName, "wt");
		int usl = uniqueSymbol(sl, lineNum);
		char tmp[MAXLEN], tmp2[MAXLEN];
		
			
		fprintf(out, "%s", sl[0].str);
		sscanf(sl[0].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
		sprintf(tmp, "%s %s %s %s", info[0], info[1], info[2], info[3]);

		for(int i=1; i<usl; i++){
			sscanf(sl[i].str, "%s %s %s %s %s %s", info[0], info[1], info[2], info[3], info[4], info[5]);
			sprintf(tmp2, "%s %s %s %s", info[0], info[1], info[2], info[3]);
			
			if( strrchr(tmp2, ' ')!=NULL ){
			  if( strcmp(tmp, tmp2)!=0 ){
				fprintf(out, "%s", sl[i].str);
				strcpy(tmp, tmp2);
			  } 
                        }
		}
		

		fclose(out);

		delete [] sl;
		sl = NULL;	
		#endif	
	}

}

#endif

void UTILITY::showArena(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum){
	int pos;
	FILE *out=openFile(fileName, "wt");
	fprintf(out, "Index Node Branch State ReDirectPath Score Hist\n");
	fprintf(out, "Arc\nModel\nWord\n");
	for(int i=0;i<showNum;i++){
		fprintf(out, "I=%d\t", i);
		fprintf(out, "N:%d\t", p2a[i]->node);
		fprintf(out, "B:%d\t", p2a[i]->branch);
		fprintf(out, "S:%d\t", p2a[i]->state);
		fprintf(out, "R:%d\t", p2a[i]->reDirectPath);
		fprintf(out, "%f\t", p2a[i]->score);

		fprintf(out, "\n\tA:");
		for(int j=0;j<h[p2a[i]->hist].pathNum;j++){
			pos=h[p2a[i]->hist].arc[j];
			fprintf(out, "%d[%d]", pos, g.arc[pos].endNode);
			(j==h[p2a[i]->hist].pathNum-1)?fprintf(out, "\n\t"):fprintf(out, "-");
		}
		// path: model
		fprintf(out, "M:");
		for(int j=0;j<h[p2a[i]->hist].pathNum;j++){
			pos=h[p2a[i]->hist].arc[j];
			(g.arc[pos].in==-1)?fprintf(out, "%s", EMPTY):fprintf(out, "%s", m.p2MMF[g.arc[pos].in]->str);
			(j==h[p2a[i]->hist].pathNum-1)?fprintf(out, "\n\t"):fprintf(out, "=");
		}
		// path: word
		fprintf(out, "W:");
		for(int j=0;j<h[p2a[i]->hist].pathNum;j++){
			pos=h[p2a[i]->hist].arc[j];
			fprintf(out, "%s", g.symbolList[g.arc[pos].out].str);
			(j==h[p2a[i]->hist].pathNum-1)?fprintf(out, "\n\t"):fprintf(out, "=");
		}
		
		#ifdef USE_LATTICE
		for(int j=0;j<h[p2a[i]->hist].pathNum;j++){
			fprintf(out, "%f[%f]", h[p2a[i]->hist].amScore[j], h[p2a[i]->hist].lmScore[j]);
			(j==h[p2a[i]->hist].pathNum-1)?fprintf(out, "\n\t"):fprintf(out, "=");			
		}
		#endif
		fprintf(out, "\n");
	}
	fclose(out);
}

void *UTILITY::aligned_malloc(size_t bytes, size_t alignment){
	if ( alignment & (alignment-1)) // make sure 'alignment' is a 2^n number
		return NULL;
	size_t size = bytes + alignment - 1;
	size += sizeof(size_t);
	void *malloc_ptr = malloc(size);        
	if (NULL == malloc_ptr)                
		return NULL;
	void *new_ptr = (void *) ((char *)malloc_ptr + sizeof(size_t));
	void *aligned_ptr = (void *) (((size_t)new_ptr + alignment - 1) & ~(alignment -1));        
	size_t delta = (size_t)aligned_ptr - (size_t)malloc_ptr;
	*((size_t *)aligned_ptr - 1) = delta;        
	return aligned_ptr;
}

void UTILITY::aligned_free(void *ptr){
	if (NULL == ptr)
		return;
	size_t delta = *( (size_t *)ptr - 1);
	void *malloc_ptr = (void *) ( (size_t)ptr - delta);
	free(malloc_ptr);
}
