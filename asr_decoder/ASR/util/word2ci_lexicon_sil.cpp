#include "recog.h"

/*
WFST_SILSP :optional sp/sil self-loop arc is appended at word-end node ( i -- sp --> j -- <F> -- i ) [Preferred]
WORDEND_SILSP_ARC :three arcs are appended in parellel toward the wordend node ( i -- sp --> j | i -- sil --> j | i -- <eps> --> j )
*/
//#define WFST_SILSP 0

class WORDTOCI {
private:
	DIC *dic;
	int dicEntryNum;
	int counter;

	LINK *link, **p2k;
	int linkNum, linkPos;
	int epsIndex;

	int computeLinkNum();
	int str2Index(char *str, int line);
	void readDic(char *inDic);
	void sortDic(DIC *d, int left, int right);
	void word2phone(int node, int branch);
	void findDic(int inStr, int *variant, int &variantNum, int node, int transition);
	int binarySearch(DIC *d, int left, int right, int inStr);
	GRAPH G;
	UTILITY U;

public:
	WORDTOCI();
	~WORDTOCI();
	int WFST_SILSP;
	void doExpand(char *inFile, char *outFile, char *inDic, char *symFile);
};

WORDTOCI::WORDTOCI(){}

WORDTOCI::~WORDTOCI(){
	if(dic!=NULL)
		delete [] dic;
	if(link!=NULL)
		delete [] link;
	if(p2k!=NULL)
		delete [] p2k;
	// graph
	G.N=NULL;
	G.NO=NULL;
	G.p2N=NULL;
	G.p2NO=NULL;
	G.arc=NULL;
	G.terminal=NULL;
	G.symbolList=NULL;
}

/**
 * @brief Return index of input string in the symbol list
 *
 * @param str Input string
 * @param line Line index of input string
 * @return Index oof symbol list
 */
int WORDTOCI::str2Index(char *str, int line){
	UTILITY util;
	int idx=util.stringSearch(G.symbolList, 0, G.symbolNum-1, str);
	if(idx==-1){
		printf("Line %d: %s not found in symbol list.\n", line, str);
		exit(1);
	}
	return idx;
}

/** 
 * @brief Sort dictionary in alphabetic order
 * @param d Dictionary to be sorted
 * @param left Lower index of sorting
 * @param right Higher index of sorting
 */
void WORDTOCI::sortDic(DIC *d, int left, int right){
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

/**
 * @brief Read dictionay on the disk
 * @param inDic Filename of dictionary
 */
void WORDTOCI::readDic(char *inDic){
	UTILITY util;
	int tmpdicEntryNum=util.getFileLineCount(inDic);
	DIC *tmpDic = new DIC[tmpdicEntryNum];	

	FILE *in=util.openFile(inDic, "rt");
	char msg[MAXLEN], chkstr[MAXLEN], *pch, *rtn, *p1, *p2;
	int j;
	for(int i=0;i<tmpdicEntryNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';

		strcpy(chkstr, msg);
		p1=strtok(chkstr, "\t "); // word
		p2=strtok(NULL, "\t "); // composition model
		if((p1==NULL)||(p2==NULL)){
			printf("Dic entry error: line %d, %s\n", i+1, msg);
			exit(1);
		}

		pch=strtok(msg, "\t ");
		// word
		tmpDic[i].word=str2Index(pch, i); // index from symbol list
		j=0;
		// phone
		memset(tmpDic[i].phone, -1, sizeof(int)*MAX_PHONE_NUM);
		pch=strtok(NULL, "\t ");
		while(pch!=NULL){ // loop to retrieve phone index
			tmpDic[i].phone[j]=str2Index(pch, i);
			j++;
			pch=strtok(NULL, "\t ");
		}
		tmpDic[i].phoneNum=j;
	}
	fclose(in);

	sortDic(tmpDic, 0, tmpdicEntryNum-1);
	
	// remove duplicated entries
	int cmp;
	for(int i=0;i<tmpdicEntryNum-1;i++){
		if(tmpDic[i].word==tmpDic[i+1].word){
			cmp=memcmp(tmpDic[i].phone, tmpDic[i+1].phone, sizeof(int)*MAX_PHONE_NUM);
			if(cmp==0)// discard entry i+1
				tmpDic[i+1].word=-1;
		}
	}
	dicEntryNum=0;
	for(int i=0;i<tmpdicEntryNum;i++){
		if(tmpDic[i].word!=-1)
			dicEntryNum++;
	}
	int d=0;
	dic = new DIC[dicEntryNum];
	for(int i=0;i<tmpdicEntryNum;i++){
		if(tmpDic[i].word!=-1){
			dic[d]=tmpDic[i];
			d++;
		}
	}
	sortDic(dic, 0, dicEntryNum-1);

	delete [] tmpDic;
}

// word2phone for j-th branch of i-th node
// An example after the word2phone expansion:
// p1:<eps>/0 p2:<eps>/0 p3:<eps>/0 p4:му/weight
void WORDTOCI::word2phone(int node, int branch){ 
	int outSymbol, endNode, variant[MAXLEN], variantNum;
	float weight;
	int pn;
	char strSIL[4]="sil";

	outSymbol=G.p2NO[node]->arc[branch].in;
	endNode=G.p2NO[node]->arc[branch].endNode;
	weight=G.p2NO[node]->arc[branch].weight;

	findDic(G.p2NO[node]->arc[branch].in, variant, variantNum, node, branch);
	for(int i=0;i<variantNum;i++){ // for each pronunication variantion
		for(int j=0;j<dic[variant[i]].phoneNum;j++){ // for each phone
			// start
			(j==0)?p2k[linkPos]->start=node:p2k[linkPos]->start=counter;  // 'counter' = 'G.nodeNum' at first
			// end
			if(j<dic[variant[i]].phoneNum-1){
				if(j==0)
					p2k[linkPos]->end=counter; 
				else{
					p2k[linkPos]->end=counter+1;
					counter++;
				}
			}
			else{ // last phone
				p2k[linkPos]->end=endNode;

				if(dic[variant[i]].phoneNum>1)// else: from node to endNode directly
					counter++;
			}
			// in/out
			p2k[linkPos]->in=dic[variant[i]].phone[j];
			pn = dic[variant[i]].phone[j];
			// outSymbol is attached to the last phone, the output labels of preceeding phones are all <eps>.
			(j<dic[variant[i]].phoneNum-1)?p2k[linkPos]->out=epsIndex:p2k[linkPos]->out=outSymbol;
			// weight
			// weight is push toward the last phone
			(j==dic[variant[i]].phoneNum-1)?p2k[linkPos]->weight=weight:p2k[linkPos]->weight=0;

			linkPos++;
		}

		if( (WFST_SILSP == 1) && ( strcmp(G.symbolList[G.p2NO[node]->arc[branch].in].str, "sil")!=0 ) ){
			( dic[variant[i]].phoneNum > 1 )? p2k[linkPos]->start=counter-1 : p2k[linkPos]->start=node;
			p2k[linkPos]->end=counter;
			p2k[linkPos]->in=pn;
			p2k[linkPos]->out=outSymbol;
			p2k[linkPos]->weight=weight;
			linkPos++;

			p2k[linkPos]->start=counter;
			p2k[linkPos]->end=endNode;
			p2k[linkPos]->in=str2Index(strSIL, -1);
			p2k[linkPos]->out=str2Index(strSIL, -1);
			p2k[linkPos]->weight=0;
			linkPos++;

			counter++;
		}

	}
}


void WORDTOCI::findDic(int inStr, int *variant, int &variantNum, int node, int transition){
	variantNum=0;
	int idx=binarySearch(dic, 0, dicEntryNum-1, inStr), direction[2]={-1,1}, pos;
	if(idx==-1){
		printf("%s does not exist in dic\n", G.symbolList[G.p2NO[node]->arc[transition].in].str);
		exit(1);
	}
	variant[variantNum]=idx;
	variantNum++;

	// 1. Because the binary search may not exactly locate the first occurrence of the given string, we need
	//    keep searching forward and backward for identical strings.
	// 2. # of variant is equal to # of times the given string was found
	for(int i=0;i<2;i++){
		pos=idx+direction[i];
		while((pos<dicEntryNum)&&(pos>=0)){
			if(dic[pos].word==dic[idx].word){
				variant[variantNum]=pos;
				variantNum++;
				pos+=direction[i];
			}
			else
				break;
		}
	}
}
/**
 * @brief Search index of input string in a dictionary
 * @param d Dictionary to be search for
 * @param left Lower index to search
 * @param right Higher index to search
 * @param inStr Input string
 * @return Index in dictionary
 */
int WORDTOCI::binarySearch(DIC *d, int left, int right, int inStr){
	int mid=(left+right)/2,difference;
	difference=inStr-d[mid].word;
	if (difference==0)
		return mid;
	(difference>0)?left=mid+1:right=mid-1;
	return ((left>right)||(right<left))?-1:binarySearch(d,left,right,inStr);
}

/**
 * @brief Compute total number of links in a WFST
 * @return Number of links
 */
int WORDTOCI::computeLinkNum(){
	// compute the # of links after considering pronunciation variations
	int variant[MAXLEN], variantNum, c=0;
	for(int i=0;i<G.nodeNum;i++){
		// .'branchNum' represents # of pronunciation variations
		for(int j=0;j<G.p2NO[i]->branchNum;j++){
			findDic(G.p2NO[i]->arc[j].in, variant, variantNum, i, j);
			// 'c' is the # of CI phones for the j-th variations
			// # of CI phones = # of arcs added in the graph

			//if( strcmp(G.symbolList[G.p2NO[i]->arc[j].in].str, "</s>")==0 )
			//	printf("</s> has %d variants, %d phones\n", variantNum, dic[variant[0]].phoneNum);

			for(int k=0;k<variantNum;k++){
				c+=dic[variant[k]].phoneNum;
				if( (WFST_SILSP==1) && (strcmp(G.symbolList[G.p2NO[i]->arc[j].in].str, "sil")!=0) )
					c+=2; // need 2 extra links to include silence
			}
		}		
	}
	return c; 
}

/**
 * @brief Expand word-level WFST to CI-phone or syllable -level WFST
 * @param inFile Input word-level WFST
 * @param outFile Output CI-phone or syllable -level WFST
 * @param inDic Dictionary
 * @param symFile Symbol list
 */
void WORDTOCI::doExpand(char *inFile, char *outFile, char *inDic, char *symFile){
	G.readGraph(inFile, symFile, 0);

	dic=NULL;
	counter=G.nodeNum;

	UTILITY util;
	epsIndex=util.stringSearch(G.symbolList, 0, G.symbolNum-1, EMPTY);
	if(epsIndex==-1){
		printf("index of %s not found.\n", EMPTY);
		exit(1);
	}

	readDic(inDic);

	// expand word-to-phone as link, including pronunciation variantions
	linkNum=computeLinkNum();
	link = new LINK[linkNum];
	p2k = new LINK*[linkNum];
	for(int i=0;i<linkNum;i++)
		p2k[i]=link+i; // copy link to p2k
	
	// do word to phone
	linkPos=0;
	for(int i=0;i<G.nodeNum;i++){
		for(int j=0;j<G.p2NO[i]->branchNum;j++)
			word2phone(i, j);
	}
	// renumerate and output
	printf("linkNum:%d linkPos:%d\n", linkNum, linkPos);
	linkNum = linkPos; // linkPos is the true number of arcs

	util.nodeRenumerate(p2k, linkNum, G.terminal, G.terminalNum);
	util.outputGraph(p2k, linkNum, G.terminal, G.terminalNum, G.symbolList, 1, outFile);
}

int main(int argc, char **argv){
	if(argc!=6){
		printf("Usage: %s graph.word graph.ci dic sym opt\n", argv[0]);
		printf("\topt=0: no word-end sil\n\topt=1: add word-end sil\n");
		exit(1);
	}
        if(atoi(argv[5])==1)
		printf("adding sil to word/syllable end likes lexicon\n");
		

	WORDTOCI w2c;
	w2c.WFST_SILSP=0;
        w2c.WFST_SILSP=atoi(argv[5]);
	if( w2c.WFST_SILSP < 0 || w2c.WFST_SILSP > 1 ){
		printf("silence option is either 0 (no sil added) or 1 (sil added)\n");
		exit(1);
	}

	w2c.doExpand(argv[1], argv[2], argv[3], argv[4]);

	return 0;
}
