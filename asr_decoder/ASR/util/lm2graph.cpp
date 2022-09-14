#include "recog.h"

#define MAX_NGRAM_ORDER 10
#define START	"<s>"
#define END	"</s>"

/**
 * @brief Structure to contain N-gram information
 */
typedef struct {
	char *str; ///< N-gram string
	int s; ///< Source string, history part of N-gram string
	int e; ///< Destination string, N-1 gram string of N-gram
	float prob; ///< N-gram probability
	float backoffProb; ///< N-gram backoff probability
} NGRAM;

class LM2GRAPH {
private:
	SYMBOLLIST *symbolList;
	int symbolNum;
	int *terminal, terminalNum;
	int fstrIndex;
	LINK *link, **p2k;
	int linkNum;
	int *auxBackoff;
	int auxBackoffNum;
	int backoff, startNode, endNode;
	int ngramNum[MAX_NGRAM_ORDER];
	int ngramOrder;
	void save2link(LINK **k, int &p, int start, int end, int in, int out, float weight);
	float direct2End(NGRAM **n, int idx, int order);
	NGRAM *ngram, **p2n;
	void sortNgram(NGRAM **n, int low, int up);
	int searchLowerOrder(NGRAM **n, int low, int up, const char *inStr);
	void findSrcDst();
	void readLMHeader(char *LMFile);
	void readLM(char *LMFile);
	void initializeData();
	void establishConnection();
	void setEntryPoint();
	void obtainSymbolList();
	int inSymbol(NGRAM **n, int idx);
public:
	LM2GRAPH();
	~LM2GRAPH();
	void doLM2Graph(char *LMFile, char *graphFile);
};

LM2GRAPH::LM2GRAPH(){}

LM2GRAPH::~LM2GRAPH(){
	if(ngram!=NULL){
		int sum=0;
		for(int i=0;i<ngramOrder;i++)
			sum+=ngramNum[i];
		for(int i=0;i<sum;i++){
			if(ngram[i].str!=NULL){
				delete [] ngram[i].str;
				ngram[i].str=NULL;
			}
		}
		delete [] ngram;
		ngram=NULL;
	}
	if(p2n!=NULL){
		delete [] p2n;
		p2n=NULL;
	}
	if(link!=NULL){
		delete [] link;
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
	if(auxBackoff!=NULL){
		delete [] auxBackoff;
		auxBackoff=NULL;
	}
}

/**
 * @brief Find index of a string in NGRAM array
 *
 * Use binary search to find the NGRAM index of a string
 *
 * @param **n Pointer to NGRAM structure
 * @param low Lower bound of index to search
 * @param up Upper bound of index to search
 * @param *inStr Input string to search its index
 * @return an NGRAM index, or -1 if doesn't find
 */
int LM2GRAPH::searchLowerOrder(NGRAM **n, int low, int up, const char *inStr){
	int mid=(low+up)/2,difference;
	difference=strcmp(inStr,n[mid]->str);
	if (difference==0)
		return mid;
	(difference>0)?low=mid+1:up=mid-1;
	return ((low>up)||(up<low))?-1:searchLowerOrder(n,low,up,inStr);
}

/**
 * @brief Connect a direct path from a higher-order node to a lower-order node.
 * 
 * When the last token of an N-gram string is </s>, the terminal symbol for an N-gram string, 
 * construct a direct link from the N-gram to the shared backoff node in the unigram level.
 *
 * @param **n Pointer to NGRAM structure
 * @param idx Index of the N-gram item
 * @param order The order of the N-gram item
 * @return Cumulated log-probabilities of this path
 */
float LM2GRAPH::direct2End(NGRAM **n, int idx, int order){
	// if the last token is equal to </s>, the weight is the summation of arcs all the way backoff to unigram.
	int p, q, r, orderCnt=order-1;
	float weight=n[idx]->prob+n[idx]->backoffProb;
	char msg[MAXLEN], *pch;

	p=0;
	for(int i=0;i<orderCnt;i++)
		p+=ngramNum[i];
	q=p+ngramNum[orderCnt]; // ex. for 'order'=1 (bigram), q=ngramNum[0]

	strcpy(msg, n[idx]->str);
	pch=strchr(msg, ' ')+1; // (N-1)-gram string, discarding the first token
	while(orderCnt>0){
		r=searchLowerOrder(n, p, q, pch);

		(r==-1)?weight+=0:weight+=n[r]->backoffProb;

		strcpy(msg, pch);
		pch=strchr(msg, ' ')+1; // lower gram string, (N-2)-gram

		orderCnt--;
		p=0;
		for(int i=0;i<orderCnt;i++)
			p+=ngramNum[i];
		q=p+ngramNum[orderCnt];
	}
	return weight;
}

/**
 * @brief Find symbol index of the last token of an N-gram string
 *
 * Tokenize an N-gram string by its last space symbol and then find the index of this token in the
 * symbol list.
 *
 * @param **n Pointer to structure NGRAM
 * @param idx Index of the N-gram item
 * @return Index in the symbol list
 */
int LM2GRAPH::inSymbol(NGRAM **n, int idx){
	UTILITY util;
	int pos;
	char msg[MAXLEN], *p;
	strcpy(msg, p2n[idx]->str);
	p=strrchr(msg, ' ');
	// 1-gram or not
	(p==NULL)?strcpy(msg, p2n[idx]->str):strcpy(msg, p+1);
	pos=util.stringSearch(symbolList, 0, symbolNum-1, msg);
	return pos;
}

/**
 * @brief Construct a symbol list from unigram strings
 *
 * Unigram strings are symbols seen in a WFST graph; so they are put into the symbol list and sorted
 * alphabetically.
 */
void LM2GRAPH::obtainSymbolList(){
	UTILITY util;
	// symbolNum is always equal to # of 1-gram
	symbolNum=ngramNum[0]+1; // add <F> : this symbol is for epsilon
	symbolList = new SYMBOLLIST[symbolNum];
	for(int i=0;i<ngramNum[0];i++)
		strcpy(symbolList[i].str, p2n[i]->str);
	strcpy(symbolList[ngramNum[0]].str, FSTR);
	util.sortString(symbolList, 0, symbolNum-1);

	fstrIndex=util.stringSearch(symbolList, 0, symbolNum-1, FSTR); // search the index of <F>
}

/**
 * @brief Find source and destination strings of an N-gram string
 *
 * Source string is the history part of an N-gram string, and destination string is (N-1)-gram string.
 * For example, when an N-gram string consists of w1 w2 w3 w4, its source string is w1 w2 w3 and its
 * destination string is w2 w3 w4.
 */
void LM2GRAPH::findSrcDst(){
	// find src and dst of ngram
	// n-gram, n>=2, since s/e of 1-gram is already known
	char *p1, *p2, msrc[MAXLEN], mdst[MAXLEN];
	int lowOrderS, lowOrderE, p, q;
	p=0;
	for(int i=1;i<ngramOrder;i++){ // start from bigram (i=1)
		p+=ngramNum[i-1]; // starting index in array 'ngramNum'
		q=p+ngramNum[i];  // ending index in array 'ngramNum'
        
		lowOrderS=p-ngramNum[i-1]; // (N-1)-gram starting index
		lowOrderE=p; // (N-1)-gram ending index

		for(int j=p;j<q;j++){
			// src string
			strcpy(msrc, p2n[j]->str); // N-gram string
			p1=strrchr(msrc, ' '); // return the location of the last space
			msrc[p1-msrc]='\0'; // now 'msrc' only contains the history part of the original N-gram string 
			// dst string
			strcpy(mdst, p2n[j]->str); // N-gram string
			p2=strchr(mdst, ' '); // return the location of the first space
			strcpy(mdst, p2+1); // 'mdsc' contains the (N-1)-gram string (discarding the first, oldest token)
			// assign node index
			// n-gram 'p2n[j]' is from its history part ((n-1)-gram) to its backoff (n-1)-gram part
			// n-gram is defined by the transition from one (n-1)-gram to another (n-1)-gram
			p2n[j]->s=searchLowerOrder(p2n, lowOrderS, lowOrderE-1, msrc); // search for 'msrc' index
			p2n[j]->e=searchLowerOrder(p2n, lowOrderS, lowOrderE-1, mdst); // search for 'mdst' index

			// the following codes are added in 2013/06/20
			//if( p2n[j]->e == -1 )
			//	printf("n-gram suffix not found in LM! %s\t%s %d -> %s %d\n", p2n[j]->str, msrc, p2n[j]->s, mdst, p2n[j]->e);

			if( (p2n[j]->e == -1) ){
				int cor, lowOS, lowOE;
				cor = i-1;
				lowOS = lowOrderS - ngramNum[cor-1];
				lowOE = lowOrderE - ngramNum[cor];

				while( p2n[j]->e == -1 ){
					if( cor == 0 ) break;
					p2=strchr(mdst, ' ');
					strcpy(mdst, p2+1);
					p2n[j]->e=searchLowerOrder(p2n, lowOS, lowOE-1, mdst);

					cor = cor-1;
					lowOS = lowOS - ngramNum[cor-1];
					lowOE = lowOE - ngramNum[cor];			
					//printf("\tsearch %s %d\n", mdst, p2n[j]->e);
				}
			}

			if( p2n[j]->s == -1 )
				printf("LAST: n-gram prefix not found in LM! %s\t%s %d -> %s %d\n", p2n[j]->str, msrc, p2n[j]->s, mdst, p2n[j]->e);
			if( p2n[j]->e == -1 )
				printf("LAST: n-gram suffix not found in LM! %s\t%s %d -> %s %d\n", p2n[j]->str, msrc, p2n[j]->s, mdst, p2n[j]->e);
							
		}
				
	}
}

/**
 * @brief Sort N-gram according to their string
 *
 * Use binary sort to sort N-gram according to their string
 *
 * @param **n Pointer to an NGRAM structure
 * @param low Lower bound of the sorting
 * @param up Upper bound of the sorting
 */
void LM2GRAPH::sortNgram(NGRAM **n, int low, int up){
	int i=low, j=up;
	char key[MAXLEN];
	NGRAM *temp;
	strcpy(key, n[(low+up)/2]->str);
	//  partition
	do{
		while (strcmp(n[i]->str,key)<0) i++;
		while (strcmp(n[j]->str,key)>0) j--;
		if (i<=j){
			temp=n[i]; n[i]=n[j]; n[j]=temp;
			i++; j--;
		}
	} while (i<=j);
	//  recursion
	if (low<j) sortNgram(n, low, j);
	if (i<up) sortNgram(n, i, up);
}

/**
 * @brief Read header of an LM file
 *
 * Read header informatin of an LM file and then calculate the total number of N-gram items.
 *
 * @param LMFile LM file
 */
void LM2GRAPH::readLMHeader(char *LMFile){
	UTILITY util;

	ngramOrder=0;
	memset(ngramNum, 0, sizeof(int)*MAX_NGRAM_ORDER);
	int fileLineNum=util.getFileLineCount(LMFile);
	char msg[MAXLEN], *pch, *rtn;	
	FILE *in=util.openFile(LMFile, "rt");
	for(int i=0;i<fileLineNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		pch=strtok(msg, "\\");
		if(pch!=NULL){//data
			rtn=fgets(msg, MAXLEN, in);
			msg[strlen(msg)-1]='\0';
			pch=strtok(msg, "=");
			pch=strtok(NULL, "=");
			while(pch!=NULL){
				if(atoi(pch)>0){
					ngramNum[ngramOrder]=atoi(pch);
					ngramOrder++;
				}
				rtn=fgets(msg, MAXLEN, in);
				msg[strlen(msg)-1]='\0';
				pch=strtok(msg, "=");
				pch=strtok(NULL, "=");
			}
			break;
		}
	}
	fclose(in);
}

/**
 * @brief Read N-gram items and extract their associated information from an LM file
 *
 * Each N-gram item has up to three fields: LM probability, N-gram string, and an optional backoff probability.
 * LM probability and backoff probability of each N-gram item are log-probabilities of base 10. During the 
 * information extraction, these log-probabilities are converted to the ones of base e (natural log).
 *
 * @param LMFile LM file
 */
void LM2GRAPH::readLM(char *LMFile){
	readLMHeader(LMFile);	
	// allocate memory
	int sum=0;
	// the total number of n-grams
	for(int i=0;i<ngramOrder;i++)
		sum+=ngramNum[i];
	ngram =new NGRAM[sum];
	p2n = new NGRAM*[sum]; // pointer to ngram
	for(int i=0;i<sum;i++)
		p2n[i]=ngram+i;
	// read as ngram
	UTILITY util;
	int order=0, c=0, fileLineNum=util.getFileLineCount(LMFile);
	FILE *in=util.openFile(LMFile, "rt");
	char msg[MAXLEN], *p1, *p2, *p3, *rtn;	
	for(int i=0;i<fileLineNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		p1=strtok(msg, "-");
		p2=strtok(NULL, "-");
		if(p2!=NULL){
			if(strcmp(p2, "grams:")==0){
				for(int j=0;j<ngramNum[order];j++){
					rtn=fgets(msg, MAXLEN, in);
					msg[strlen(msg)-1]='\0';
					p1=strtok(msg, "\t"); // log-probability
					p2=strtok(NULL, "\t"); // n-gram string
					p3=strtok(NULL, "\t"); // backoff probability (not always exists!)
					
					// original prob is log10 based, new one is Ln based
					p2n[c]->prob=log((float)10)*(float)atof(p1); 
					p2n[c]->str=new char[strlen(p2)+1];
					strcpy(p2n[c]->str, p2);
					(p3!=NULL)?p2n[c]->backoffProb=log((float)10)*(float)atof(p3):p2n[c]->backoffProb=log((float)10)*0;
					c++;
				}
				order++;
				if(order==ngramOrder)
					break;
			}
		}
	}
	fclose(in);
	// sort ngram (by its string)
	// each n-gram is sorted seperately
	int p, q;
	for(int i=0;i<ngramOrder;i++){
		(i==0)?p=0:p+=ngramNum[i-1];
		q=p+ngramNum[i];
		sortNgram(p2n, p, q-1);
	}
	// set <s> as start, </s> as end
	// search for the indexes of <s> and </s>
	int s, e;
	s=searchLowerOrder(p2n, 0, ngramNum[0]-1, START);
	if(s==-1){
		printf("%s not found\n", START);
		exit(1);
	}
	e=searchLowerOrder(p2n, 0, ngramNum[0]-1, END);
	if(e==-1){
		printf("%s not found\n", END);
		exit(1);
	}
	startNode=s;
	endNode=e;
}

/**
 * @brief Update LINK information of a link
 * 
 * @param **k Pointer to link structure
 * @param &p Array index of a LINK array
 * @param start Source string index of an N-gram item
 * @param end Destination string index of an N-gram item
 * @param in Input label index
 * @param out Output label index
 * @param weight LM weight of the link
 */
void LM2GRAPH::save2link(LINK **k, int &p, int start, int end, int in, int out, float weight){
	k[p]->start=start;
	k[p]->end=end;
	k[p]->in=in;
	k[p]->out=out;
	k[p]->weight=weight;
	//printf("%d\t%d\t%s\t%f\n", start, end, symbolList[out].str, weight);
	p++;
}

/**
 * @brief Initialize necessary memory usage in building a WFST graph
 * 
 * Allocate memory space for structure of LINK, pointer array to LINK, and auxBackoff.
 */
void LM2GRAPH::initializeData(){
	// represent as links
	linkNum=0;
	// each n-gram arc has three associated information
	for(int i=0;i<ngramOrder;i++)
		linkNum+=(ngramNum[i]*3); // prob and backoffProb*2 (another auxiliary backoffprob)
	link = new LINK[linkNum];
	p2k = new LINK*[linkNum];
	for(int i=0;i<linkNum;i++)
		p2k[i]=link+i;
	// obtain backoff index
	int nNum=0;
	for(int i=0;i<ngramOrder;i++)
		nNum+=ngramNum[i]; // total number of n-gram
	auxBackoffNum=nNum; // each n-gram item has its own auxBackoff
	auxBackoff=new int[auxBackoffNum];
	for(int i=0;i<auxBackoffNum;i++){
		auxBackoff[i]=nNum; // index is from nNum to 2*nNum, the first nNum are reserved for regular n-gram items
		nNum++;
	}

	backoff=nNum;// the "one and only one" backoff node, its index is 2*nNum
}
/**
 * @brief Connect N-gram items with each other by their relationship
 * 
 * Connect N-gram items and their associated backoff by their relationship. Unigram is connected first, then
 * bigram is connected on the top of unigram. The steps repeat until the highest order of N-gram items are 
 * connected. Notice that when source or destination string of a N-gram item is missing, the N-gram item
 * will not be included in the final WFST graph.
 */
void LM2GRAPH::establishConnection(){
	initializeData();

	char mend[MAXLEN], *pch;
	float weight;
	int p, q, c=0; 
	// 1-gram
	for(int i=0;i<ngramNum[0];i++){
		if(i!=endNode){// 1-gram -> backoff; 'endNode' is the index of '</s>'
			// i!=endNode because </s> doesn't need to backoff
			// link 1-gram to the backoff state with <F>:<F>/p2n[i]->backoffProb, where <F> is the epsilon symbol
			save2link(p2k, c, i, auxBackoff[i], fstrIndex, fstrIndex, p2n[i]->backoffProb);
			// link auxBackoff[i] to the backoff state with <F>:<F>/0
			save2link(p2k, c, auxBackoff[i], backoff, fstrIndex, fstrIndex, 0);
		}
		// startNode only has output links
		if(i!=startNode)// backoff -> 1-gram; 'startNode' is the index of '<s>';
			save2link(p2k, c, backoff, i, inSymbol(p2n, i), inSymbol(p2n, i), p2n[i]->prob);
	}
	//printf("1-gram OK\n");
	// (n-2)-gram -> (n-1)-gram -> (n-2)-gram
	p=0;
	for(int i=1;i<ngramOrder-1;i++){ // 2-gram to (N-1)-gram		
		p+=ngramNum[i-1];
		q=p+ngramNum[i];
		for(int j=p;j<q;j++){
			if((p2n[j]->s!=-1)&&(p2n[j]->e!=-1)){// src and dst exist
				strcpy(mend, p2n[j]->str);
				pch=strrchr(mend, ' ')+1; // retrieve the last token of the string
				if(strcmp(pch, END)!=0){ // != '</s>'
					// p2n[j]->s is the index of history part of an N-gram string
					save2link(p2k, c, p2n[j]->s, j, inSymbol(p2n, j), inSymbol(p2n, j), p2n[j]->prob);
					save2link(p2k, c, j, auxBackoff[j], fstrIndex, fstrIndex, p2n[j]->backoffProb);
					// 'p2n[j]->e' points to the node representing (N-1)-gram of the node 'j'
					save2link(p2k, c, auxBackoff[j], p2n[j]->e, fstrIndex, fstrIndex, 0);
				}
				else{ // last token is </s> (pch == '</s>')
					weight=direct2End(p2n, j, i);
					save2link(p2k, c, p2n[j]->s, endNode, inSymbol(p2n, j), inSymbol(p2n, j), weight);
				}
			}
			//printf("j=%d\n", j);
		}
		//printf("%d-gram OK\n", i+1);
	}
	// (n-1)-gram -> (n-1)-gram
	// dealing with the highest order of n-gram (linking the nodes within (n-1)-gram
	p=0;
	for(int i=0;i<ngramOrder-1;i++)
		p+=ngramNum[i];
	// p is the total number of sum_{1_gram count}^{n-1_gram count} items
	q=p+ngramNum[ngramOrder-1]; // q is sum_{1_gram count}^{n_gram count}
	for(int i=p;i<q;i++){
		if((p2n[i]->s!=-1)&&(p2n[i]->e!=-1)){// src and dst exist
			strcpy(mend, p2n[i]->str);
			pch=strrchr(mend, ' ')+1; // retrieve the last token of the string
			if(strcmp(pch, END)!=0){ 
				if(p2n[i]->s==p2n[i]->e){ // when this happens?
					save2link(p2k, c, p2n[i]->s, i, inSymbol(p2n, i), inSymbol(p2n, i), p2n[i]->prob);
					save2link(p2k, c, i, auxBackoff[i], fstrIndex, fstrIndex, p2n[i]->backoffProb);
					save2link(p2k, c, auxBackoff[i], p2n[i]->e, fstrIndex, fstrIndex, 0);
				}
				else{ 
					// links between two (N-1)-grams
					save2link(p2k, c, p2n[i]->s, p2n[i]->e, inSymbol(p2n, i), inSymbol(p2n, i), p2n[i]->prob);
				}
			}
			else{
				weight=direct2End(p2n, i, ngramOrder-1);
				save2link(p2k, c, p2n[i]->s, endNode, inSymbol(p2n, i), inSymbol(p2n, i), weight);
			}
		}
	}	
	linkNum=c;

	setEntryPoint();
}

/**
 * @brief Set entry point of a WFST graph
 * 
 * Make sure the entry point of a WFST graph is indexed as 0.
 */
void LM2GRAPH::setEntryPoint(){
	// make sure startNode=0 (index is zero)
	if(startNode!=0){// swap node index
		for(int i=0;i<linkNum;i++){
			if(p2k[i]->start==startNode)
				p2k[i]->start=-1;
			if(p2k[i]->end==startNode)
				p2k[i]->end=-1;
		}
		for(int i=0;i<linkNum;i++){
			if(p2k[i]->start==0)
				p2k[i]->start=startNode;
			if(p2k[i]->end==0)
				p2k[i]->end=startNode;
		}
		for(int i=0;i<linkNum;i++){
			if(p2k[i]->start==-1)
				p2k[i]->start=0;
			if(p2k[i]->end==-1)
				p2k[i]->end=0;
		}
	}
	// terminal
	terminalNum=1;
	terminal=new int[terminalNum];
	(endNode==0)?terminal[0]=startNode:terminal[0]=endNode;
}
/**
 * @brief lm2graph main function
 *
 * Convert a SRILM toolkit generated language model to a WFST graph in word level.
 *
 * @param LMFile Language model file
 * @param graphFile WFST graph file
 */
void LM2GRAPH::doLM2Graph(char *LMFile, char *graphFile){
	readLM(LMFile);
	obtainSymbolList();

	findSrcDst();
	establishConnection();

	UTILITY util;
	util.nodeRenumerate(p2k, linkNum, terminal, terminalNum);
	util.outputGraph(p2k, linkNum, terminal, terminalNum, symbolList, 0, graphFile);
}

/**
 * @brief Entry of the function
 */
int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s n-gramLM.in word-levelGraph.out\n", argv[0]);
		exit(1);
	}
	
	LM2GRAPH lm2graph;
	lm2graph.doLM2Graph(argv[1], argv[2]);

	return 0;
}
