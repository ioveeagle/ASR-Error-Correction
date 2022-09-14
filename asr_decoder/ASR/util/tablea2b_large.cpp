#include "recog.h"

#define MAX_PATH_LEN	12 // a redirected path should not consist of more than 12 arcs

typedef struct {
	int endNode;
	float prob;
	int pathNum; // how many paths in this redirected path
	int *path;	
} REDIRECT;

typedef struct {
	int reDirectNum;
	REDIRECT *rd;
} LOOKUPTABLE;

class TABLEA2B {
private:
	LOOKUPTABLE *lt;
	int nodeNum;
	void checkData();
	int reDirectPathNum(char *inStr);
	void splitPathString(int NodeIndex, int reDirectIndex, char *inStr);
public:
	TABLEA2B();
	~TABLEA2B();
	void readLookupTable(char *file);
	void write2Bin(char *file);
};

TABLEA2B::TABLEA2B(){
	lt=NULL;
}

TABLEA2B::~TABLEA2B(){
	if(lt!=NULL){
		for(int i=0;i<nodeNum;i++){
			if(lt[i].reDirectNum>0){
				for(int j=0;j<lt[i].reDirectNum;j++)
					delete [] lt[i].rd[j].path;
				delete [] lt[i].rd;
			}
		}
		delete [] lt;
	}
}

int TABLEA2B::reDirectPathNum(char *inStr){
	int num=0, len=(int)strlen(inStr);
	for(int i=0;i<len;i++)
		if(inStr[i]=='.')
			num++;
	return num+1;
}

void TABLEA2B::splitPathString(int NodeIndex, int reDirectIndex, char *inStr){
	int count=0, q=0, len;
	char str[MAXLEN], term[MAXLEN];
	strcpy(str, inStr);
	len=strlen(str);
	for(int i=0;i<len;i++){
		term[q]=str[i];
		q++;
		if(str[i]=='.'){
			q--;
			term[q]='\0';			
		}
		if((str[i]=='.')||(i==len-1)){
			term[q]='\0';
			lt[NodeIndex].rd[reDirectIndex].path[count]=atoi(term);
			count++;
			q=0;
		}
	}
}

void TABLEA2B::readLookupTable(char *file){
	UTILITY util;
	nodeNum=util.getFileLineCount(file);
	
	lt = new LOOKUPTABLE[nodeNum];
	FILE *in=util.openFile(file, "rt");
	char msg[MAXLEN*320], temp[MAXLEN], *pch, pathStr[MAXLEN], *rtn;
	int endNode, pathNum;
	float prob;
	for(int i=0;i<nodeNum;i++){
		rtn=fgets(msg, MAXLEN*320, in);
		msg[strlen(msg)-1]='\0';
		pch=strtok(msg, " ");	// reDirectNum
		lt[i].reDirectNum=atoi(pch); // 0 or a positive integer
		if(lt[i].reDirectNum>0){
			lt[i].rd = new REDIRECT[lt[i].reDirectNum];
			for(int j=0;j<lt[i].reDirectNum;j++){
				pch=strtok(NULL, " ");
				strcpy(temp, pch);
				// Example: 
				// 2 300574,0.000000,0.0 42456,0.000000,0.0.0.0 				
			  // 2 redirect pathes
			  // 300574,0.000000,0.0: next node Index, Prob, pathStr
				sscanf(temp, "%d,%f,%s", &endNode, &prob, pathStr);
				lt[i].rd[j].endNode=endNode;
				lt[i].rd[j].prob=(float)log((double)pow((float)10,prob));

				pathNum=reDirectPathNum(pathStr); // how many digits enclose the dot symbols "."
				lt[i].rd[j].pathNum=pathNum;
				lt[i].rd[j].path = new int[pathNum];
				splitPathString(i, j, pathStr); // split information is stored in lt[i].rd[j].path[]
			}
		}
		//printf("N=%d of %d\n", i, nodeNum);
	}
	fclose(in);

	checkData();
}

void TABLEA2B::checkData(){
	// check data
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<lt[i].reDirectNum;j++){
			if(lt[i].rd[j].pathNum>MAX_PATH_LEN){
				printf("Endnode %d, pathNum %d is greater than MAX_PATH_LEN %d.\n", i, lt[i].rd[j].pathNum, MAX_PATH_LEN);
				exit(1);
			}
		}
	}
}

void TABLEA2B::write2Bin(char *file){
	UTILITY util;

	FILE *out=util.openFile(file, "wb");
	int ary1Size, ary2Size;
	unsigned int ary3Size, ary4Size;
	ary1Size=nodeNum;
	ary2Size=nodeNum;
	ary3Size=0;
	for(int i=0;i<nodeNum;i++)
		ary3Size+=lt[i].reDirectNum;
	ary4Size=ary3Size;

	// header
	fwrite(&ary1Size, sizeof(int), 1, out);//number of endNodes=nodeNum
	fwrite(&ary2Size, sizeof(int), 1, out);//size of array position
	fwrite(&ary3Size, sizeof(unsigned int), 1, out);//set of endNodes
	fwrite(&ary4Size, sizeof(unsigned int), 1, out);//set of probs

	short int *LTendNodeNum = new short int[ary1Size];
	//short int *LTendNodeArrayPosition = new short int[ary2Size]; // DANGER FOR LARGE GRAPH!
	unsigned int *LTendNodeArrayPosition = new unsigned int[ary2Size]; // @20120130 Chi-Yueh Lin

	int *LTendNodeArray = new int[ary3Size];
	float *LTProb = new float[ary4Size];

	for(int i=0;i<nodeNum;i++) // # of redirect paths from node 'i'
		LTendNodeNum[i]=lt[i].reDirectNum;
	
	LTendNodeArrayPosition[0]=0; // redirect information index for node 'i'
	for(int i=1;i<nodeNum;i++)
		LTendNodeArrayPosition[i]=LTendNodeArrayPosition[i-1]+lt[i-1].reDirectNum;

	printf("%s::%s()::Last LTendNodeArrayPosition: %d\n", __FILE__, __func__, LTendNodeArrayPosition[nodeNum-1]);

	// debug
	//for(int i=0;i<nodeNum;i++){
	//	LTendNodeArrayPosition[i]=0;
	//	for(int j=0;j<i;j++)
	//		LTendNodeArrayPosition[i]+=lt[j].reDirectNum;
	//}
	//for(int i=0;i<20;i++)
	//	printf("%d\n", LTendNodeArrayPosition[i]);
	//exit(1);

	int idx=0;
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<lt[i].reDirectNum;j++){
			LTendNodeArray[idx]=lt[i].rd[j].endNode; // the endNode index for j-th path of i-th node
			LTProb[idx]=lt[i].rd[j].prob;
			//printf("%d %f\n", idx, LTProb[idx]); //DDD
			idx++;
		}
	}

	// body
	// LTendNodeNum: # of redirect paths from node 'i'
	// LTendNodeArrayPosition: redirect information index for node 'i'
	fwrite(LTendNodeNum, sizeof(short int), ary1Size, out);
	//fwrite(LTendNodeArrayPosition, sizeof(short int), ary2Size, out);
	fwrite(LTendNodeArrayPosition, sizeof(unsigned int), ary2Size, out); // for LVCSR
 	fwrite(LTendNodeArray, sizeof(int), ary3Size, out);
	fwrite(LTProb, sizeof(float), ary4Size, out);
	fclose(out);

	delete [] LTendNodeNum;
	delete [] LTendNodeArrayPosition;
	delete [] LTendNodeArray;
	delete [] LTProb;
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s table.txt table.bin\n", argv[0]);
		exit(1);
	}

	char tableTxt[MAXLEN], tableBin[MAXLEN];
	strcpy(tableTxt, argv[1]);
	strcpy(tableBin, argv[2]);

	TABLEA2B ta2b;
	ta2b.readLookupTable(tableTxt);
	ta2b.write2Bin(tableBin);

	return 0;
}
