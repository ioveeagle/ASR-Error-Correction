#include "recog.h"

typedef struct {
	int start;
	int end;
	int phoneNum;
	int in[MAX_PHONE_NUM];
	int duplicatedFrom;
	int firstBranch; // @20120801
} EXTRACT;

typedef struct {
	int duplicatedFrom;
	int duplicatedNum;
	int partialExpand;
	int *extractedPos;
} UNIQUEENTRY;

class PARTIALEXPAND {
private:
	LINK *link;
	int linkNum;
	int *validLink;
	EXTRACT *extract;
	int extractNum;
	UNIQUEENTRY *uniqueEntry;
	int uniqueEntryNum;
	int MAX_EXTRACT_NUM;
	NODE_OFFLINE **p2NO;
	int nodeNum;
	int arcNum;
	SYMBOLLIST *symbolList;
	int symbolNum;
	int *wordEnd;
	int epsIndex;
	int *mapNode;
	int *terminal;
	int terminalNum;
	void determineExpand(int ratio);
	void sortUniqueEntry(UNIQUEENTRY *ue, int s, int e);
	void findUniqueEntry();
	void determineWordEnd();
	void toWordEnd(int node, int branch);
	void obtainExtractedArc();
	void crossLink();
	void outputData(char *outdic, char *outGraph);
	void renumerate();
public:
	PARTIALEXPAND(GRAPH &graph);
	~PARTIALEXPAND();
	void doPartialExpand(char *outDic, char *outGraph, int ratio);
};

PARTIALEXPAND::PARTIALEXPAND(GRAPH &graph){
	p2NO=graph.p2NO;
	nodeNum=graph.nodeNum;
	link = new LINK[graph.arcNum];
	arcNum = graph.arcNum;
	terminal=graph.terminal;
	terminalNum=graph.terminalNum;

	wordEnd = new int[nodeNum];

	symbolList=graph.symbolList;
	symbolNum=graph.symbolNum;
	UTILITY util;
	epsIndex=util.stringSearch(symbolList, 0, symbolNum-1, EMPTY);
	if(epsIndex==-1){
		printf("index of %s not found.\n", EMPTY);
		exit(1);
	}
}

PARTIALEXPAND::~PARTIALEXPAND(){
	if(link!=NULL)
		delete [] link;
	if(wordEnd!=NULL)
		delete [] wordEnd;
	if(uniqueEntry!=NULL){
		for(int i=0;i<uniqueEntryNum;i++){
			if(uniqueEntry[i].extractedPos!=NULL)
				delete uniqueEntry[i].extractedPos;
		}
		delete [] uniqueEntry;
	}
	if(extract!=NULL)
		delete [] extract;
	if(mapNode!=NULL)
		delete [] mapNode;
	if(validLink!=NULL)
		delete [] validLink;
}

void PARTIALEXPAND::toWordEnd(int node, int branch){
	int path[MAX_PHONE_NUM], phoneNum=0, stopHere=0, endNode, n=node, b=branch;
	int cond1, cond2, cond3;
	memset(path, -1, sizeof(int)*MAX_PHONE_NUM);
	
	extract[extractNum].start=node;
	extract[extractNum].phoneNum=0;
	while(stopHere==0){
		endNode=p2NO[n]->arc[b].endNode;		
		
    // typical labels for phones, excluding the last phone in a word
		if((p2NO[n]->arc[b].in!=epsIndex)&&(p2NO[n]->arc[b].out==epsIndex)){			
			path[phoneNum]=p2NO[n]->arc[b].in;
			if( phoneNum==0 ) extract[extractNum].firstBranch = b; // @20120801
			extract[extractNum].end=endNode;			
			phoneNum++;			
		}

		(wordEnd[endNode]==1)?cond1=1:cond1=0; // the last arc in a word?
		(p2NO[endNode]->branchNum>1)?cond2=1:cond2=0; // the next node has multiple branches?
		(p2NO[n]->arc[b].out!=epsIndex)?cond3=1:cond3=0; // non-eps output labels?
		
		if( cond1||cond2||cond3 )
			stopHere=1;
		else{
			n=endNode;
			b=0;
		}
	}

	if(phoneNum>1){
		extract[extractNum].phoneNum=phoneNum;
		memcpy(extract[extractNum].in, path, sizeof(int)*MAX_PHONE_NUM);
		extractNum++;
	}
}

void PARTIALEXPAND::determineWordEnd(){
	// get inDegree
	int *inDegree=new int[nodeNum];
	memset(inDegree, 0, sizeof(int)*nodeNum);
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++){
			inDegree[p2NO[i]->arc[j].endNode]++;
		}
	}
	// word end, check wether node 'i' represents word end
	// node 'i' has inDegree larger than 1 => wordEnd
	UTILITY util;
	memset(wordEnd, 0, sizeof(int)*nodeNum);
	wordEnd[0]=1;
	for(int i=0;i<nodeNum;i++){
		if(util.isTerminal(terminal, terminalNum, i)==true)
			wordEnd[i]=1;
		else if(inDegree[i]>1)
			wordEnd[i]=1;
		else{ // node with inDegree=1
			for(int j=0;j<p2NO[i]->branchNum;j++){
				if(p2NO[i]->arc[j].out!=epsIndex)
					wordEnd[p2NO[i]->arc[j].endNode]=1;
				if(p2NO[i]->arc[j].in==epsIndex) 
					wordEnd[i]=1;
			}
		}
	}

	delete [] inDegree;
}

void PARTIALEXPAND::sortUniqueEntry(UNIQUEENTRY *ue, int s, int e){
	int i=s, j=e, key=ue[(s+e)/2].duplicatedNum;
	UNIQUEENTRY temp[1];
	//  partition
	do{
		while (ue[i].duplicatedNum-key>0) i++;
		while (ue[j].duplicatedNum-key<0) j--;
		if (i<=j){
			temp[0]=ue[i]; ue[i]=ue[j]; ue[j]=temp[0];
			i++; j--;
		}
	} while (i<=j);

	//  recursion
	if (s<j) sortUniqueEntry(ue, s, j);
	if (i<e) sortUniqueEntry(ue, i, e);
}


void PARTIALEXPAND::findUniqueEntry(){
	// check the same path
	int n;
	for(int i=0;i<extractNum;i++)
		extract[i].duplicatedFrom=-1;
	for(int i=0;i<extractNum;i++){		
		if(extract[i].duplicatedFrom==-1){
			extract[i].duplicatedFrom=i;
			for(int j=i+1;j<extractNum;j++){
				n=memcmp(extract[i].in, extract[j].in, sizeof(int)*MAX_PHONE_NUM);
				if(n==0)
					extract[j].duplicatedFrom=i; // j-th path is the same as i-th path
			}
		}
	}
	// uniqueEntry number
	UNIQUEENTRY *tmp = new UNIQUEENTRY[extractNum];
	for(int i=0;i<extractNum;i++){
		tmp[i].duplicatedNum=0;
		tmp[i].extractedPos=NULL;
	}
	for(int i=0;i<extractNum;i++){
		tmp[extract[i].duplicatedFrom].duplicatedNum++;
		tmp[i].duplicatedFrom=extract[i].duplicatedFrom;
	}
	uniqueEntryNum=0;
	for(int i=0;i<extractNum;i++){
		if(tmp[i].duplicatedNum!=0)
			uniqueEntryNum++;
	}
	// allocate uniqueEntry
	uniqueEntry = new UNIQUEENTRY[uniqueEntryNum];
	int p=0;
	for(int i=0;i<extractNum;i++){
		if(tmp[i].duplicatedNum!=0){
			uniqueEntry[p].extractedPos = new int[tmp[i].duplicatedNum];
			uniqueEntry[p].duplicatedFrom=tmp[i].duplicatedFrom;
			uniqueEntry[p].duplicatedNum=0; // will be increased in the followings
			p++;
		}
	}
	// get occurrence pos
	for(int i=0;i<uniqueEntryNum;i++){
		for(int j=0;j<extractNum;j++){
			if(extract[j].duplicatedFrom==uniqueEntry[i].duplicatedFrom){
				uniqueEntry[i].extractedPos[uniqueEntry[i].duplicatedNum]=j;
				uniqueEntry[i].duplicatedNum++;
			}
		}
	}
	sortUniqueEntry(uniqueEntry, 0, uniqueEntryNum-1);
	// release memory
	if(tmp!=NULL)
		delete [] tmp;
}

void PARTIALEXPAND::determineExpand(int ratio){
	int occurrenceSum=0, tempSum=0;
	float threshold=(float)ratio/5, tempRatio;
	for(int i=0;i<uniqueEntryNum;i++){
		if(uniqueEntry[i].duplicatedNum>1)
			occurrenceSum+=uniqueEntry[i].duplicatedNum;
	}

	for(int i=0;i<uniqueEntryNum;i++){
		tempSum+=uniqueEntry[i].duplicatedNum;
		tempRatio=(float)tempSum/occurrenceSum; // < 1 at first, but 'tempSum' gradually increases afterwards
		if(tempRatio<=threshold)
			uniqueEntry[i].partialExpand=1;
		else
			uniqueEntry[i].partialExpand=0;
	}
}

void PARTIALEXPAND::crossLink(){
	// convert to link
	linkNum=0;
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++){
			link[linkNum].start=i;
			link[linkNum].end=p2NO[i]->arc[j].endNode;
			link[linkNum].in=p2NO[i]->arc[j].in;
			link[linkNum].out=p2NO[i]->arc[j].out;
			link[linkNum].weight=p2NO[i]->arc[j].weight;
			
			linkNum++;			
		}
		
	}
	validLink = new int[linkNum];
	memset(validLink, 0, sizeof(int)*linkNum);
	// determine cross-link
	int tmpNode, *n=new int[nodeNum], pos;
	memset(n, 0, sizeof(int)*nodeNum);
	for(int i=0;i<uniqueEntryNum;i++){
		if(uniqueEntry[i].partialExpand==1){			
			for(int j=0;j<uniqueEntry[i].duplicatedNum;j++){
				pos=uniqueEntry[i].extractedPos[j];
				tmpNode=extract[pos].start;
				n[tmpNode]=1;
				
				
				//for(int k=0;k<extract[j].phoneNum;k++){ 
				for(int k=0;k<extract[pos].phoneNum;k++){ // @20120801
					if( k==0 ) // @20120801
						tmpNode=p2NO[tmpNode]->arc[extract[pos].firstBranch].endNode; // @20120801
					else // @20120801
						tmpNode=p2NO[tmpNode]->arc[0].endNode;

					n[tmpNode]=1;
					
					//if( (tmpNode==0)||(tmpNode==1) ) printf("i=%d j=%d %d %d %d\n", i, j, k, tmpNode, extract[j].phoneNum);
				}
				
			}
		}
	}
	for(int i=0;i<linkNum;i++){
		//if( i==0 ) printf("%d %d %d %d\n", n[link[i].start], n[link[i].end], link[i].start, link[i].end);
		if( (n[link[i].start]==1)&&(n[link[i].end]==1) )
			validLink[i]=1;			
		else
			validLink[i]=0;
	}
	delete [] n;
}

void PARTIALEXPAND::renumerate(){
	UTILITY util;
	int cnt=0, s, e;
	mapNode = new int[nodeNum];
	memset(mapNode, -1, sizeof(int)*nodeNum);
	// renumerate of link
	for(int i=0;i<linkNum;i++){
		s=link[i].start;
		e=link[i].end;
		if(validLink[i]==0){
			if(mapNode[s]==-1){
				mapNode[s]=cnt;
				cnt++;
			}
			if(util.isTerminal(terminal, terminalNum, e)==false){
				if(mapNode[e]==-1){
					mapNode[e]=cnt;
					cnt++;
				}
			}
			else
				mapNode[e]=-100;
		}
		//if( i<2 ) printf("%d, %d %d -> %d\n", i, validLink[i], mapNode[s], mapNode[e]);
	}
	// renumerate of extract
	for(int i=0;i<uniqueEntryNum;i++){
		if(uniqueEntry[i].partialExpand==1){
			for(int j=0;j<uniqueEntry[i].duplicatedNum;j++){
				int p=uniqueEntry[i].extractedPos[j];
				if(mapNode[extract[p].start]==-1){
					mapNode[extract[p].start]=cnt;
					cnt++;
				}
				if(mapNode[extract[p].end]==-1){
					mapNode[extract[p].end]=cnt;
					cnt++;
				}
			}
		}
	}
	// so as to assign terminal(s) the largest number
	terminalNum=0;
	for(int i=0;i<nodeNum;i++){
		if(mapNode[i]==-100){
			mapNode[i]=cnt;
			terminal[terminalNum]=cnt;
			terminalNum++;
			cnt++;
		}
	}
}

void PARTIALEXPAND::outputData(char *outdic, char *outGraph){
	UTILITY util;
	FILE *out=util.openFile(outGraph, "wt");
	int s, e, symIn, symOut;
	// output of ordinary link
	for(int i=0;i<linkNum;i++){
		s=mapNode[link[i].start];
		e=mapNode[link[i].end];
		if((s!=-1)&&(e!=-1)){
			symIn=link[i].in;
			symOut=link[i].out;
			fprintf(out, "%d\t%d\t%s\t%s", s, e, symbolList[symIn].str, symbolList[symOut].str);
			if(link[i].weight==0)
				fprintf(out, "\n");
			else
				fprintf(out, "\t%f\n", (float)(log10(exp(link[i].weight))));
		}
	}

	// output of extract
	int pos, tmp;
	float weight;
	for(int i=0;i<uniqueEntryNum;i++){
		if(uniqueEntry[i].partialExpand==1){
			for(int j=0;j<uniqueEntry[i].duplicatedNum;j++){
				pos=uniqueEntry[i].extractedPos[j];
				s=mapNode[extract[pos].start];
				e=mapNode[extract[pos].end];
				fprintf(out, "%d\t%d\tD=%d\t%s", s, e, i, EMPTY);
				//fprintf(out, "\t=> pos=%d\t%d, %d", pos, extract[pos].start, extract[pos].end);

				weight=0;
				tmp=extract[pos].start;
				for(int j=0;j<extract[pos].phoneNum;j++){
					weight+=p2NO[tmp]->arc[0].weight;
					tmp=p2NO[tmp]->arc[0].endNode;
				}
				if(weight==0)
					fprintf(out, "\n");
				else
					fprintf(out, "\t%f\n", (float)(log10(exp(link[i].weight))));
			}
		}
	}
	for(int i=0;i<terminalNum;i++)
		fprintf(out, "%d\n", terminal[i]);
	fclose(out);

	// output dic.xp
	// merge multiple phones as a single entry in the dic
	int p;
	out=util.openFile(outdic, "wt");
	char msg[MAXLEN];
	for(int i=0;i<uniqueEntryNum;i++){
		if(uniqueEntry[i].partialExpand==1){
			p=uniqueEntry[i].duplicatedFrom;
			strcpy(msg, "");
			for(int j=0;j<extract[p].phoneNum;j++){
				strcat(msg, symbolList[extract[p].in[j]].str);
				strcat(msg, "\t");
			}
			msg[strlen(msg)-1]='\0';
			fprintf(out, "%s\n", msg);
		}
	}
	fclose(out);
}

void PARTIALEXPAND::obtainExtractedArc(){
	//extract = new EXTRACT[arcNum]; // 'arcNum' is too large....balloning the RAM
	extract = new EXTRACT[arcNum/10];
	extractNum=0;
	for(int i=0;i<nodeNum;i++){
		//if( i<=2 ) printf("%d\n", wordEnd[i]);
		if(wordEnd[i]==1){
			for(int j=0;j<p2NO[i]->branchNum;j++){
				toWordEnd(i, j);
			}
		}
	}
	printf("arcNum=%d extract=%d extractNum=%d\n", arcNum, arcNum/10, extractNum);

	EXTRACT *tmp=new EXTRACT[extractNum];
	for(int i=0;i<extractNum;i++)
		tmp[i]=extract[i];
	if(extract!=NULL)
		delete [] extract;

	extract = new EXTRACT[extractNum];
	for(int i=0;i<extractNum;i++)
		extract[i]=tmp[i];
	delete [] tmp;
}

void PARTIALEXPAND::doPartialExpand(char *outDic, char *outGraph, int ratio){
	determineWordEnd(); // check whether a node represents the end of a word
        printf("determineWordEnd() ok\n");
	obtainExtractedArc();
        printf("obtainExtractedArc() ok\n");
	findUniqueEntry();
        printf("findUniqueEntry() ok\n");
	determineExpand(ratio);
        printf("determineExpand() ok\n");

	crossLink();
        printf("crossLink() ok\n");
	renumerate();
        printf("renumerate() ok\n");
	outputData(outDic, outGraph);
        printf("obtainExtractedArc() ok\n");
}

int main(int argc, char **argv){
	if(argc!=6){
		printf("Usage: %s g.in g.out dic.ext sym ratio\n", argv[0]);
		printf("expand ratio=0~5, max~min\n");
		exit(1);
	}
	char inGraph[MAXLEN], outGraph[MAXLEN], outDic[MAXLEN], symbolFile[MAXLEN];
	strcpy(inGraph, argv[1]);
	strcpy(outGraph, argv[2]);
	strcpy(outDic, argv[3]);
	strcpy(symbolFile, argv[4]);
	
	if((atoi(argv[5])<0)||(atoi(argv[5])>5)){
		printf("ratio=0,1,2,3,4,5\n");
		exit(1);
	}
	
	GRAPH graph;
	graph.readGraph(inGraph, symbolFile, 1); // the last augment denotes partialexpand!
	printf("ReadGraph() done! %ld nodes, %ld arcs.\n", graph.nodeNum, graph.arcNum);
	//exit(0);
	PARTIALEXPAND pe(graph);
 	printf("PARTIALEXPAND object initialized!\n");
	pe.doPartialExpand(outDic, outGraph, atoi(argv[5]));

	return 0;
}
