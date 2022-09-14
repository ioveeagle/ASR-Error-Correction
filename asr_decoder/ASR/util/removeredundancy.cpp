#include "recog.h"

#define SET_CLEAR -1

typedef struct {
	int in; // index of input label from 'sym'
	int out; // index of output label from 'sym'
	float weight;
	int endNode; // transit to which node
	int tablePos;
} ENCODEDARC;

typedef struct {
	int branchNum; // output transition
	int inDegree; // input transition
	int removed;
	int *arcPos;
} GRAPHNODE;

class REMOVEREDUNDANCY {
private:
	int operation;
	int machineType;
	int *terminal, terminalNum;

	SYMBOLLIST *symbolList, *encodeTable;
	int symbolNum;

	XINT encodeTableSize;
	XINT linkNum;
	XINT *arcCompare;
	XINT nodeNum, arcNum;
	XINT addedNode;

	LINK *link, **p2k;
	

	GRAPHNODE *gn;
	ENCODEDARC *ea;
	
	
	

	void readGraph(char *graphFile);
	void getTransitionFormat(char *str);
	void obtainEncodeTable();
	void encodeGraph(char *graphFile);
	void freeMemory(void *block);
	void uniqueBranch(int node);
	int determineRedundancy(XINT node, int *keepBranch);
	XINT splitNode(XINT node, int toBranch, XINT eTo);
	void treeAttach(XINT fromNode, XINT toNode, XINT node, int branch);
	void removeRedundancy(XINT node, int uniqueBranchNum);
	void addTerminal(XINT fromNode, XINT toNode);
	void updateTerminal();
	void decodeGraph();
public:
	REMOVEREDUNDANCY();
	~REMOVEREDUNDANCY();
	void doRemoveRedundancy(char *inFile, char *outFile, char *symFile, int option);
};

REMOVEREDUNDANCY::REMOVEREDUNDANCY(){}

REMOVEREDUNDANCY::~REMOVEREDUNDANCY(){
	if(symbolList!=NULL){
		delete [] symbolList;
		symbolList=NULL;
	}
	if(arcCompare!=NULL){
		delete [] arcCompare;
		arcCompare=NULL;
	}
	freeMemory(encodeTable);
	freeMemory(terminal);
	if(link!=NULL){
		delete [] link;
		link=NULL;
	}
	if(p2k!=NULL){
		delete [] p2k;
		p2k=NULL;
	}
	if(ea!=NULL){
		delete [] ea;
		ea=NULL;
	}
	if(gn!=NULL){
		XINT i;
		for(i=0;i<nodeNum;i++)
			freeMemory(gn[i].arcPos);
		freeMemory(gn);
	}
}

void REMOVEREDUNDANCY::freeMemory(void *block){
	if(block!=NULL){
		free(block);
		block=NULL;
	}
}

void REMOVEREDUNDANCY::getTransitionFormat(char *str){
	UTILITY util;
	int s, e, in, out;
	float weight;
	char msg[MAXLEN], *p1, *p2, *p3, *p4, *p5;

	strcpy(msg, str);
	p1=strtok(msg, "\t ");
	p2=strtok(NULL, "\t ");
	p3=strtok(NULL, "\t ");
	p4=strtok(NULL, "\t ");
	p5=strtok(NULL, "\t ");
	// start
	s=atoi(p1);
	// end
	if(p2!=NULL)
		e=atoi(p2);
	// in (input label)
	if(p2!=NULL){
		in=util.stringSearch(symbolList, 0, symbolNum-1, p3);
		if(in==-1){
			printf("%s, %s not found in symbolList\n", str, p3);
			exit(1);
		}
	}
	// out (output label)
	if(p2!=NULL){
		if(machineType==1){ // transducer
			out=util.stringSearch(symbolList, 0, symbolNum-1, p4);
			if(out==-1){
				printf("%s, %s not found in symbolList\n", str, p4);
				exit(1);
			}
		}
		else // acceptor
			out=in;
	}
	// weight
	if(p2!=NULL){
		if(machineType==1) // transducer
			(p5==NULL)?weight=0:weight=(float)atof(p5);
		else // acceptor
			(p4==NULL)?weight=0:weight=(float)atof(p4);
	}
	weight=(float)log((double)pow((float)10,weight));
	// terminal
	if(p2==NULL){
		terminal=(int*)realloc(terminal, sizeof(int)*(terminalNum+1));
		terminal[terminalNum]=s;
		terminalNum++;
	}
	// save to arc
	if(p2!=NULL){ // existing p2 denotes a complete arc
		// arcNum is equal to # of lines in a graph file
		ea[arcNum].in=in;
		ea[arcNum].out=out;
		ea[arcNum].endNode=e;
		ea[arcNum].weight=weight;

		gn[s].arcPos=(int*)realloc(gn[s].arcPos, sizeof(int)*(gn[s].branchNum+1));
		gn[s].arcPos[gn[s].branchNum]=arcNum;

		gn[s].branchNum++;
		arcNum++;
	}
}

void REMOVEREDUNDANCY::readGraph(char *graphFile){
	UTILITY util;
	typedef struct{
		char str[150];
	}T;
	int fileLineNum, segmentSize=20000, c;
	T *t=new T[segmentSize];

	// graph info
	util.getGraphInfo(graphFile, fileLineNum, nodeNum, arcNum, machineType);
	// allocate and initialize memory
	gn=(GRAPHNODE*)malloc(nodeNum*sizeof(GRAPHNODE));
	
	for(int i=0;i<nodeNum;i++){		
		gn[i].branchNum=0;
		gn[i].arcPos=(int*)malloc(gn[i].branchNum*sizeof(int));
	}
	ea = new ENCODEDARC[arcNum];
	
	arcNum=0; // reset as counter
	terminalNum=0;
	terminal=(int*)malloc(terminalNum*sizeof(int));
	// read into memory
	c=0;
	char msg[MAXLEN];
	FILE *in=util.openFile(graphFile, "rt");
	for(int i=0;i<fileLineNum;i++){
		fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		strcpy(t[c].str, msg);
		c++;
		if((c==segmentSize)||(i==fileLineNum-1)){
			for(int j=0;j<c;j++) // loop for each transition
				getTransitionFormat(t[j].str);
			c=0;
		}
	}
	fclose(in);
	
	// release memory
	if(t!=NULL){
		delete [] t;
		t=NULL;
	}
}

void REMOVEREDUNDANCY::obtainEncodeTable(){
	UTILITY util;
	
	XINT i;
	XINT c=0, p;

	int j;

	int segmentSize=2000000;
	char msg[MAXLEN];

	encodeTableSize=0;
	encodeTable=(SYMBOLLIST*)malloc(encodeTableSize*sizeof(SYMBOLLIST));
	SYMBOLLIST *et=new SYMBOLLIST[segmentSize];
	for(i=0;i<nodeNum;i++){
		for(j=0;j<gn[i].branchNum;j++){
			p=gn[i].arcPos[j]; // p is an index pointed to a specific arc
			
			// input label index, output label index, weight
			// for acceptor, ea[p].in == ea[p].out
			sprintf(msg, "%d:%d:%f", ea[p].in, ea[p].out, ea[p].weight); 
			if(util.stringSearch(encodeTable, 0, encodeTableSize-1, msg)==-1){ // if msg is not in the encodeTable yet
				strcpy(et[c].str, msg); // only unique 'msg' goes here
				c++;
				if(c==segmentSize){				
					c=util.uniqueSymbol(et, c); // 'c' is the # of unique symbols so far
					encodeTable=(SYMBOLLIST*)realloc(encodeTable, sizeof(SYMBOLLIST)*(encodeTableSize+c));
					// memcpy(dst, src, num)
					memcpy(encodeTable+encodeTableSize, et, sizeof(SYMBOLLIST)*c);
					encodeTableSize+=c;
					util.sortString(encodeTable, 0, encodeTableSize-1);
					c=0;// reset counter
					#ifdef X64
					printf("encodeTableSize is %ld @Node %ld/%ld\n", encodeTableSize, i, nodeNum);
					#else
					printf("encodeTableSize is %d @Node %d/%d\n", encodeTableSize, i, nodeNum); // Added by Chi-Yueh Lin @20110804
					#endif
				}
			}
		}
	}
	if(c!=0){
		c=util.uniqueSymbol(et, c); // 'c' is the # of unique symbols so far
		encodeTable=(SYMBOLLIST*)realloc(encodeTable, sizeof(SYMBOLLIST)*(encodeTableSize+c));
		memcpy(encodeTable+encodeTableSize, et, sizeof(SYMBOLLIST)*c);
		encodeTableSize+=c;
		util.sortString(encodeTable, 0, encodeTableSize-1);
		c=0;// reset counter
	}
	// release memory
	if(et!=NULL){
		delete [] et;
		et=NULL;
	}
}

void REMOVEREDUNDANCY::encodeGraph(char *graphFile){
	XINT i;
	XINT e, p, q;

	int j;

	readGraph(graphFile);
	//printf("readGraph ok\n"); // Added by Chi-Yueh Lin @20110804
	obtainEncodeTable();
	printf("obtainEncodeTable() ok\n"); // Added by Chi-Yueh Lin @20110804
	// do encoding
	UTILITY util;
	char msg[MAXLEN];
	
	for(i=0;i<nodeNum;i++){
		gn[i].inDegree=0;
		gn[i].removed=0; // if gn[i].removed=1, it will be removed in the following steps
	}
	for(i=0;i<nodeNum;i++){
		for(j=0;j<gn[i].branchNum;j++){
			p=gn[i].arcPos[j]; // index to a specific arc j
			e=ea[p].endNode; // the arc 'p' transit to node 'e'
			gn[e].inDegree++; // the number of input arcs in node 'e' 

			sprintf(msg, "%d:%d:%f", ea[p].in, ea[p].out, ea[p].weight); // encode the arc msg
			q=util.stringSearch(encodeTable, 0, encodeTableSize-1, msg); // return 'msg' index in encodeTable
			ea[p].tablePos=q; // arc p is associated with q-th line in the encodeTable
		}
	}
	// for arc comparison
	arcCompare = new XINT[encodeTableSize];
}

void REMOVEREDUNDANCY::removeRedundancy(XINT node, int uniqueBranchNum){
	int  c, branchNum=gn[node].branchNum;
	XINT *arcBackup=NULL;
 
	if(branchNum>uniqueBranchNum){// meaning that some arcs are the same
		// backup
		arcBackup=(XINT*)malloc(branchNum*sizeof(XINT));
		memcpy(arcBackup, gn[node].arcPos, sizeof(XINT)*branchNum);
		freeMemory(gn[node].arcPos);
		// get non-SET_CLEAR transitions
		gn[node].arcPos=(XINT*)malloc(uniqueBranchNum*sizeof(XINT));
		c=0;
		for(int i=0;i<branchNum;i++){
			if(arcBackup[i]!=SET_CLEAR){
				gn[node].arcPos[c]=arcBackup[i];
				c++;
			}
		}
		gn[node].branchNum=uniqueBranchNum;
		freeMemory(arcBackup);
	}
}

XINT REMOVEREDUNDANCY::splitNode(XINT node, int toBranch, XINT eTo){
	int branchNum=gn[eTo].branchNum;
	XINT toNode;
	if(gn[eTo].inDegree>1){
		// replace branch
		// at beginning, 'addedNode' = 'numNode' of the original graph
		// the reference path has been copied, and the ending node is indexed 'addedNode'
		ea[gn[node].arcPos[toBranch]].endNode=addedNode; 
		// copy info
		gn=(GRAPHNODE*)realloc(gn, sizeof(GRAPHNODE)*(addedNode+1));
		gn[addedNode].removed=0;
		gn[addedNode].branchNum=branchNum;
		gn[addedNode].arcPos=(int*)malloc(branchNum*sizeof(int));		
		gn[addedNode].inDegree=1;
		memcpy(gn[addedNode].arcPos, gn[eTo].arcPos, branchNum*sizeof(int));
		// the node to copy to
		toNode=addedNode; // 'addedNode' is also a node index
		// a new node is split (copy)
		addedNode++;
	}
	else // the easiest case, gn[eTo] only has one input transition
		toNode=eTo;
	return toNode;
}
// treeAttach(eFrom, eTo, node, i); 
// 'eTo': 'addedNode'
// 'i': branch index of 'node'
void REMOVEREDUNDANCY::treeAttach(XINT fromNode, XINT toNode, XINT node, int branch){
	int p, branchNum=gn[fromNode].branchNum+gn[toNode].branchNum;
	// re-allocate memory
	// expand 'addedNode' arc number
	gn[toNode].arcPos=(int*)realloc(gn[toNode].arcPos, sizeof(int)*branchNum);
	// attach tree-fromNode to tree-toNode
	p=gn[toNode].branchNum;
	// copy branches of gn[eFrom] and append them to gn[addedNode]
	// 'gn[toNode].arcPos+p' because the first p arcs are already determined in splitNode()
	memcpy(gn[toNode].arcPos+p, gn[fromNode].arcPos, sizeof(int)*gn[fromNode].branchNum);
	gn[toNode].branchNum=branchNum; // update gn[addedNode] branch number
	// update inDegree
	if(gn[fromNode].inDegree>1){
		for(int d=0;d<gn[fromNode].branchNum;d++)
			gn[ea[gn[fromNode].arcPos[d]].endNode].inDegree++;
	}
	gn[fromNode].inDegree--; // the duplicate arc will be removed, so 'inDegree' decrease by 1
	// remove 'gn[fromNode]' only if there is no more input transition.
	// if gn[fromNode].inDegree > 1, gn[fromNode] will survive
	if(gn[fromNode].inDegree==0) 
		gn[fromNode].removed=1;
	// check terminal
	addTerminal(fromNode, toNode);
}

void REMOVEREDUNDANCY::addTerminal(XINT fromNode, XINT toNode){
	int found=-1;
	XINT *tmp=NULL;
	for(int i=0;i<terminalNum;i++){
		if(fromNode==terminal[i]){
			found=i;
			break;
		}
	}

  // if 'fromNode' belongs to terminal nodes, create a new terminal node
	if(found!=-1){
		tmp=new XINT[terminalNum+1];
		memcpy(tmp, terminal, sizeof(XINT)*terminalNum);
		// since 'fromNode' (a terminal node) will be removed, the 'toNode' should be a terminal node.
		tmp[terminalNum]=toNode;
		terminalNum++;
		if(terminal!=NULL)
			delete [] terminal;
		terminal = new XINT[terminalNum];
		memcpy(terminal, tmp, sizeof(XINT)*terminalNum);
		if(tmp!=NULL)
			delete [] tmp;
	}
}

int REMOVEREDUNDANCY::determineRedundancy(XINT node, int *keepBranch){
	int uniqueBranchNum=0, endNode;
	XINT p;

	// arc has been compared?
	for(int i=0;i<gn[node].branchNum;i++)
		arcCompare[ea[gn[node].arcPos[i]].tablePos]=SET_CLEAR;

	if(operation==0){ // remove redundant branches
		for(int i=0;i<gn[node].branchNum;i++){
			// find the index of arc p in the table
			// If several arcs have identical labels (in:out:weight), they point to the same p value
			p=ea[gn[node].arcPos[i]].tablePos; 
			if(arcCompare[p]==SET_CLEAR){ // arc p has not been visited (SET_CLEAR=-1)
				arcCompare[p]=i; // mark a non -1 value, meaning that arc p has been visited
				keepBranch[i]=i; // mark a non -1 value
				uniqueBranchNum++;
			}
			else // if arc p has been visited, don't increase 'uniqueBranchNum'
				keepBranch[i]=arcCompare[p]; // keepBranch[i] != i anymore!
		}
	}
	else{ // merge equivalent paths
		for(int i=0;i<gn[node].branchNum;i++){
			p=ea[gn[node].arcPos[i]].tablePos;
			endNode=ea[gn[node].arcPos[i]].endNode;
			if((arcCompare[p]==SET_CLEAR)||(gn[endNode].inDegree>1)){
				arcCompare[p]=i;
				keepBranch[i]=i;
				uniqueBranchNum++;
			}
			else
				keepBranch[i]=arcCompare[p];
		}
	}

	return uniqueBranchNum;
}

void REMOVEREDUNDANCY::uniqueBranch(XINT node){

	int srcBranch, uniqueBranchNum, *keepBranch=NULL;
	XINT eFrom, eTo, nodeCount=0, *node2makeUnique=NULL;

	// if a node gn[node] only has 1 output transition, there is no duplicate label problem.
	// Thus we don't need to perform the following manipulation.
	// just skip gn[node], process next node
	if((gn[node].removed==0)&&(gn[node].branchNum>1)){
		node2makeUnique = new XINT[gn[node].branchNum]; // record which node has to be processed
		keepBranch = new int[gn[node].branchNum];
				
		// check whether output transitions of 'node' are unique
		uniqueBranchNum=determineRedundancy(node, keepBranch);

		//printf("\t\tnode:%d\tuniqueBranch:%d\tbranch:%d\n", node, uniqueBranchNum, gn[node].branchNum);
		// take unique branches
		for(int i=0;i<gn[node].branchNum;i++){
			// These 'keepBranch[i]!=i' branches have to be removed!
			// keepBranch[i]=i : do nothing
			if(keepBranch[i]!=i){
				srcBranch=keepBranch[i]; // which node does this branch transit to
				eTo=ea[gn[node].arcPos[srcBranch]].endNode; // dst node of the reference branch
				eFrom=ea[gn[node].arcPos[i]].endNode; // dst node of the need-to-be-removed branch
				
				if(eTo!=eFrom){
					// (1) if gn[eTo].inDegree = 1, the returned 'eTo' is equal to input 'eTo'. Thus NO splitNode
					// (2) if gn[eTo].inDegree > 1, make a copy of gn[eTo] and term it gn[addedNode]
					//     but gn[addedNode].inDegree = 1 (however, gn[addedNode].branchNum = gn[eTo].branchNum)
					//     in this case, returned 'eTo' = 'addedNode'
					eTo=splitNode(node, srcBranch, eTo);
					treeAttach(eFrom, eTo, node, i);
					node2makeUnique[nodeCount]=eTo;
					nodeCount++;
				}
				gn[node].arcPos[i]=SET_CLEAR; // mark for removal
			}			
		} // all branches of 'node' have been processed
		removeRedundancy(node, uniqueBranchNum); // remove arcs with gn[node].arcPos[] == -1 (SET_CLEAR)
		// take unique branch(es) of involved node(s)
		for(XINT i=0;i<nodeCount;i++){
			if((node2makeUnique[i]<node)||(node2makeUnique[i]>=nodeNum))
				uniqueBranch(node2makeUnique[i]); 
		}
		// release memory
		if(keepBranch!=NULL)
			delete [] keepBranch;
		if(node2makeUnique!=NULL)
			delete [] node2makeUnique;
	}
}

void REMOVEREDUNDANCY::updateTerminal(){
	int c=0;
	XINT *tmp=new XINT[terminalNum];
	// backup terminals
	memset(tmp, -1, sizeof(XINT)*terminalNum);
	for(int i=0;i<terminalNum;i++){
		if(gn[terminal[i]].removed==0){
			tmp[c]=terminal[i];
			c++;
		}
	}
	if(terminal!=NULL)
		delete [] terminal;
	// new terminals
	terminal = new XINT[c];	
	memcpy(terminal, tmp, sizeof(XINT)*c);
	terminalNum=c;

	if(tmp!=NULL){
		delete [] tmp;
		tmp=NULL;
	}
}

void REMOVEREDUNDANCY::doRemoveRedundancy(char *inFile, char *outFile, char *symFile, int option){
	operation=option;
	XINT i;

	UTILITY util;
	// read symbol
	symbolNum=util.getFileLineCount(symFile);
	symbolList = new SYMBOLLIST[symbolNum];
	util.readSymbolList(symbolList, symFile, symbolNum);
	
	encodeGraph(inFile);
  // printf("encodeGraph (in doRemoveRedundancy) ok\n"); // Added by Chi-Yueh Lin @20110804
  // In other words, 'addedNode' is the total number of nodes in the graph so far.
  // As new node created, 'addedNode' increases by 1 accordingly. Thus 'addedNode' can be regarded
  // as maximal node index.
	addedNode=nodeNum; 
	for(i=0;i<nodeNum;i++){
		// check unique output transitions of each node i
		// uniqueBranch() is executed recursively (see the detail inside the function)
		uniqueBranch(i); 
		//printf("\tDoing uniqueBranch(%d/%d)\n", i, nodeNum); // Added by Chi-Yueh Lin @20110804
	}
	
	updateTerminal();
	decodeGraph();

	util.nodeRenumerate(p2k, linkNum, terminal, terminalNum);
	util.outputGraph(p2k, linkNum, terminal, terminalNum, symbolList, machineType, outFile);
}

void REMOVEREDUNDANCY::decodeGraph(){
	// update 'nodeNum': total # of nodes in the graph
	nodeNum=addedNode;
	XINT i;

	// g2e -> link
	linkNum=0;
	for(i=0;i<nodeNum;i++){
		if(gn[i].removed==0)
			linkNum+=gn[i].branchNum; // total # of arcs in the graph
	}
	printf("decodeGraph():%d links\n", linkNum);
	link=new LINK[linkNum]; 
	XINT c=0;
	for(i=0;i<nodeNum;i++){
		if(gn[i].removed==0){
			for(int j=0;j<gn[i].branchNum;j++){
				link[c].start=i;
				link[c].end=ea[gn[i].arcPos[j]].endNode;
				link[c].in=ea[gn[i].arcPos[j]].in;
				link[c].out=ea[gn[i].arcPos[j]].out;
				link[c].weight=ea[gn[i].arcPos[j]].weight;
				c++;
			}
		}
	}
	// using **p2k for faster access
	// thus p2k just a look-up table containing the address of each 'link'
	p2k = new LINK *[linkNum]; 
	for(i=0;i<linkNum;i++)
		p2k[i]=link+i;
}

int main(int argc, char **argv){
	if(argc!=5){
		printf("Usage: %s graph.in graph.out sym option\n", argv[0]);
		printf("option\n0:remove redundant branches\n1:merge equivalent paths (on reversed graph)\n");
		exit(1);
	}
	char inFile[MAXLEN], outFile[MAXLEN], symbolFile[MAXLEN];
	strcpy(inFile, argv[1]);
	strcpy(outFile, argv[2]);
	strcpy(symbolFile, argv[3]);
	int option=atoi(argv[4]);
	if((option!=0)&&(option!=1)){
		printf("Error assigning option: 0 or 1\n");
		exit(1);
	}

	REMOVEREDUNDANCY mt;
	mt.doRemoveRedundancy(inFile, outFile, symbolFile, option);

	return 0;
}
