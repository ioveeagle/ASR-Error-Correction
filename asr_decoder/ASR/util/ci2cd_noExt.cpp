#include "recog.h"

#include <algorithm>
#include <vector>

using namespace std;

typedef struct {
	int uniqueSymNum;
	int *sym;
	int *node;
} CONTEXT;

class CITOCD {
private:
	NODE_OFFLINE **p2NO;
	SYMBOLLIST *symbolList;
	int symbolNum;
	int nodeNum, arcNum, *terminal, terminalNum, machineType;

	int epsIndex;
	CONTEXT *lcxt;
	LINK *link, **p2k;
	int linkNum;
	bool zeroIndegreeStart;
	SYMBOLLIST *CDList;
	int CDNum;
	void getCDList();
	void obtainAllInSym();
	void addInSym(CONTEXT *cxt, int node, int CISym);
	void inOutConnection();
	void expandContext(int node);
	void checkStartEnd();
	void updateSymbolList();
public:
	CITOCD(GRAPH &graph);
	~CITOCD();
	void doci2cd(char *outFile);
};

CITOCD::CITOCD(GRAPH &graph){
	nodeNum=graph.nodeNum;
	arcNum=graph.arcNum;
	terminalNum=graph.terminalNum;
	terminal=new int[terminalNum];
	memcpy(terminal, graph.terminal, sizeof(int)*terminalNum);
	machineType=graph.machineType;
	p2NO=graph.p2NO;
	symbolNum=graph.symbolNum;
	symbolList = new SYMBOLLIST[symbolNum];
	memcpy(symbolList, graph.symbolList, sizeof(SYMBOLLIST)*symbolNum);

	UTILITY util;
	epsIndex=util.stringSearch(symbolList, 0, symbolNum-1, EMPTY);
}

CITOCD::~CITOCD(){
	if(terminal!=NULL){
		delete [] terminal;
		terminal=NULL;
	}
	if(lcxt!=NULL){
		for(int i=0;i<nodeNum;i++){
			free(lcxt[i].sym);
			free(lcxt[i].node);
			lcxt[i].sym=NULL;
			lcxt[i].node=NULL;
		}
		free(lcxt);
		lcxt=NULL;
	}
	if(link!=NULL){
		free(link);
		link=NULL;
	}
	if(p2k!=NULL){
		delete [] p2k;
		p2k=NULL;
	}
	if(symbolList!=NULL){
		delete [] symbolList;
		symbolList=NULL;
	}
	if(CDList!=NULL){
		delete [] CDList;
		CDList=NULL;
	}
}

// obtain All-Input-Symbol (CI-phone symbol)
void CITOCD::obtainAllInSym(){
	int s, e;
	// obtain sym
	for(int i=0;i<nodeNum;i++){
		s=i; // doesn't use
		for(int j=0;j<p2NO[i]->branchNum;j++){
			e=p2NO[i]->arc[j].endNode;
			// adding input symbol of the link (i->e) for node 'e'
			// thus the "left-context" of node 'e'
			addInSym(lcxt, e, p2NO[i]->arc[j].in);
		}
	}
	
	// if starting node has no input links, add <eps> as its left-context.
	if(lcxt[0].uniqueSymNum==0){
		zeroIndegreeStart=true;
		addInSym(lcxt, 0, epsIndex);
	}
	else
		zeroIndegreeStart=false;
}

void CITOCD::addInSym(CONTEXT *cxt, int node, int CISym){
	// notice that 'uniqueSymNum' is not the # of unique symbols in this step,
	// it just contains all CI symbols appear in the input links of node 'e', aka left-context of node 'e'
	cxt[node].sym=(int*)realloc(cxt[node].sym, sizeof(int)*(cxt[node].uniqueSymNum+1));
	cxt[node].sym[cxt[node].uniqueSymNum]=CISym;
	cxt[node].uniqueSymNum++;
}

void CITOCD::inOutConnection(){
	// initialize
	lcxt=(CONTEXT*)malloc(nodeNum*sizeof(CONTEXT));	 // left-context
	for(int i=0;i<nodeNum;i++){
		lcxt[i].uniqueSymNum=0;
		lcxt[i].sym=(int*)malloc(lcxt[i].uniqueSymNum*sizeof(int));
		lcxt[i].node=(int*)malloc(lcxt[i].uniqueSymNum*sizeof(int));
	}

	obtainAllInSym(); // obtain all input symbols
	
	// 'symbolNum' = # of unique CI phones
	// 'symCmp' := symbol has been compared? initialized by -1
	int num, c=0, *symCmp=new int[symbolNum], n, rightBranchNum;
	memset(symCmp, -1, sizeof(int)*symbolNum);
	
	for(int i=0;i<nodeNum;i++){
		// obtain uniqueSymNum
		num=0; // count unique left-context symbols for node 'i'
		// check left-contexts of  node 'i'
		for(int j=0;j<lcxt[i].uniqueSymNum;j++){
			// if symCmp[lcxt[i].sym[j]] is equal to -1, which means that this symbol is unique.
			// thus 'num' denotes the # of unique symbols in the graph
			if(symCmp[lcxt[i].sym[j]]==-1){ // symbol has been used?
				symCmp[lcxt[i].sym[j]]=lcxt[i].sym[j]; // symbol index 'lcxt[i].sym[j]' has been used
				num++; // increase unique symbol count by 1
			}
		}
		// re-allocate memory for sym
		free(lcxt[i].sym);
		lcxt[i].uniqueSymNum=0;
		lcxt[i].sym=(int*)malloc(sizeof(int)*num); // reallocate memory for unique symbols
		for(int j=0;j<symbolNum;j++){
			if(symCmp[j]!=-1){ // unique symbols have non -1 value
				lcxt[i].sym[lcxt[i].uniqueSymNum]=j; // record unique symbol index
				lcxt[i].uniqueSymNum++;
			}
		}
		// allocate memory for node
		// check right-context of node 'i'
		(p2NO[i]->branchNum==0)?rightBranchNum=1:rightBranchNum=p2NO[i]->branchNum;
		n=0;
		// 'lcxt[i].uniqueSymNum*rightBranchNum' := all input/output label combinations for node 'i'
		lcxt[i].node=(int*)malloc(sizeof(int)*(lcxt[i].uniqueSymNum*rightBranchNum));
		for(int j=0;j<symbolNum;j++){
			if(symCmp[j]!=-1){
				for(int k=0;k<rightBranchNum;k++){
					lcxt[i].node[n]=c; // ???
					n++; // local counter for each node
					c++; // global counter
				}
			}
		}
		// reset for next node
		for(int j=0;j<lcxt[i].uniqueSymNum;j++)
			symCmp[lcxt[i].sym[j]]=-1;
	}
	delete [] symCmp;
	symCmp=NULL;
}

void CITOCD::expandContext(int node){
	UTILITY util;
	char msg[MAXLEN], pL[100], pC[100], pR[100];
	// 'LMR[]' store symbol index for left-context, middle-context, and right-context
	int LMR[3], s=node, e, p, rightBranch;
	
	for(int i=0;i<lcxt[s].uniqueSymNum;i++){// unique left-context
		for(int j=0;j<p2NO[s]->branchNum;j++){// middle-context
			e=p2NO[s]->arc[j].endNode;
			(p2NO[e]->branchNum==0)?rightBranch=1:rightBranch=p2NO[e]->branchNum; // # of right-context
			for(int k=0;k<rightBranch;k++){ //right-context
				// retrieve labels on the link
				LMR[0]=lcxt[s].sym[i]; // unique left-context of node 's'
				LMR[1]=p2NO[s]->arc[j].in; // middle-context of node 's'
				(p2NO[e]->branchNum==0)?LMR[2]=epsIndex:LMR[2]=p2NO[e]->arc[k].in; // right-context of node 'e'
				// save to link
				link=(LINK*)realloc(link, sizeof(LINK)*(linkNum+1));
				
				link[linkNum].start=lcxt[s].node[i*p2NO[s]->branchNum+j]; // ???
				// determine unique symbols of middle-context
				for(int m=0;m<lcxt[e].uniqueSymNum;m++){
					if(lcxt[e].sym[m]==p2NO[s]->arc[j].in){
						p=m;
						break;
					}
				}
				link[linkNum].end=lcxt[e].node[p*p2NO[e]->branchNum+k]; // ???

				sprintf(msg, "%s-%s+%s", symbolList[LMR[0]].str, symbolList[LMR[1]].str, symbolList[LMR[2]].str);

				link[linkNum].in=util.stringSearch(CDList, 0, CDNum-1, msg); // CI lables are expanded into CD labels
				link[linkNum].out=p2NO[s]->arc[j].out; // output symbols are kept the same
				link[linkNum].weight=p2NO[s]->arc[j].weight; // weights are kept the same

				linkNum++;
			}
		}
	}
}

void CITOCD::checkStartEnd(){
	int S, E;
	// single start	
	if(zeroIndegreeStart==true){
		S=0;
		E=p2NO[0]->branchNum;
		for(int i=0;i<linkNum;i++){
			if(link[i].start<E)
				link[i].start=0;
			if(link[i].end<E)
				link[i].end=0;
		}
	}
	else{
		S=0;
		E=lcxt[0].uniqueSymNum*p2NO[0]->branchNum;
		for(int i=S+1;i<E;i++){
			link=(LINK*)realloc(link, sizeof(LINK)*(linkNum+1));
			link[linkNum].start=0;
			link[linkNum].end=i;
			link[linkNum].in=epsIndex;
			link[linkNum].out=epsIndex;
			link[linkNum].weight=0;
			linkNum++;
		}
	}
	// update terminals
	int *t, tNum=0, tNode;
	t=(int*)malloc(tNum*sizeof(int));
	for(int i=0;i<terminalNum;i++){
		tNode=terminal[i];
		(p2NO[tNode]->branchNum>0)?E=lcxt[tNode].uniqueSymNum*p2NO[tNode]->branchNum:E=1;
		for(int j=0;j<E;j++){
			t=(int*)realloc(t, sizeof(int)*(tNum+1));
			t[tNum]=lcxt[tNode].node[j];
			tNum++;
		}
	}
	delete [] terminal;
	terminal = new int[tNum];
	memcpy(terminal, t, sizeof(int)*tNum);
	terminalNum=tNum;
	free(t);
}

void CITOCD::updateSymbolList(){
	UTILITY util;
	// backup
	SYMBOLLIST *bk=new SYMBOLLIST[symbolNum];
	memcpy(bk, symbolList, sizeof(SYMBOLLIST)*symbolNum);
	int bkNum=symbolNum;
	// update symbolList to cover new CD strings
	int slNum=symbolNum+CDNum;
	SYMBOLLIST *sl = new SYMBOLLIST[slNum];
	memcpy(sl, symbolList, sizeof(SYMBOLLIST)*symbolNum);
	memcpy(sl+symbolNum, CDList, sizeof(SYMBOLLIST)*CDNum);
	slNum=util.uniqueSymbol(sl, slNum);
	
	delete [] symbolList;
	symbolNum=slNum;
	symbolList = new SYMBOLLIST[symbolNum];
	memcpy(symbolList, sl, sizeof(SYMBOLLIST)*symbolNum);	
	delete [] sl;
	sl=NULL;
	// renew symbol index
	int p;
	for(int i=0;i<linkNum;i++){
		// in
		p=p2k[i]->in; // encoded by CDList
		p2k[i]->in=util.stringSearch(symbolList, 0, symbolNum-1, CDList[p].str);
		// out
		p=p2k[i]->out;// encoded by bk
		p2k[i]->out=util.stringSearch(symbolList, 0, symbolNum-1, bk[p].str);
	}
	// release memory
	if(bk!=NULL){
		delete [] bk;
		bk=NULL;
	}
}

void CITOCD::getCDList(){
	// CI
	vector<int> vectCI;
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++){
			vectCI.push_back(p2NO[i]->arc[j].in); // gather all CI index
		}
	}
	vectCI.push_back(epsIndex); // add <eps>
	sort(vectCI.begin(), vectCI.end());
	vector<int>::iterator it=unique(vectCI.begin(), vectCI.end());
	vectCI.erase(it, vectCI.end());
	int CINum=vectCI.size();
	// CD
	CDNum=CINum*CINum*CINum; // get all CD combinations from # of CIs
	CDList = new SYMBOLLIST[CDNum];
	char msg[MAXLEN], A[MAXLEN], B[MAXLEN], C[MAXLEN];
	// generate all CD-phone labels into 'CDList'
	// Notice that these CD-phone labels are based on CI-phone labels. The ones which don't
	// appear in CI-phone will not appear in CD-phone as well.
	for(int i=0;i<CINum;i++){
		for(int j=0;j<CINum;j++){
			for(int k=0;k<CINum;k++){
				strcpy(A, symbolList[vectCI[i]].str);
				strcpy(B, symbolList[vectCI[j]].str);
				strcpy(C, symbolList[vectCI[k]].str);
				sprintf(msg, "%s-%s+%s", A, B, C);
				strcpy(CDList[i*CINum*CINum+j*CINum+k].str, msg);
			}
		}
	}
	UTILITY util;
	util.sortString(CDList, 0, CDNum-1);
}

void CITOCD::doci2cd(char *outFile){
	getCDList(); // find # of CIs and use it to form CDs
	inOutConnection(); 

	linkNum=0;
	link=(LINK*)malloc(linkNum*sizeof(LINK));

	for(int i=0;i<nodeNum;i++)
		expandContext(i);
	checkStartEnd();
	
	p2k=new LINK*[linkNum];
	for(int i=0;i<linkNum;i++)
		p2k[i]=link+i;

	updateSymbolList();

	UTILITY util;
	util.nodeRenumerate(p2k, linkNum, terminal, terminalNum);
	util.outputGraph(p2k, linkNum, terminal, terminalNum, symbolList, machineType, outFile);
}

// Expanding CI labels into CD labels, notice that the expansion is CROSS WORD BOUNDARY
int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage: %s ci.in cd.out sym\n", argv[0]);
		exit(1);
	}
	char inFile[MAXLEN], outFile[MAXLEN], symFile[MAXLEN];
	strcpy(inFile, argv[1]);
	strcpy(outFile, argv[2]);
	strcpy(symFile, argv[3]);

	GRAPH graph;
	graph.readGraph(inFile, symFile, 0);

	CITOCD ci2cd(graph);
	ci2cd.doci2cd(outFile);

	return 0;
}
