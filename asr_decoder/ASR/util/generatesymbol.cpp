#include "recog.h"
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>

using namespace std;

class GENERATESYMBOL{
private:
	vector<string> vect;
	void readFile(char *dicFile);
public:
	GENERATESYMBOL();
	~GENERATESYMBOL();
	void doGenerateSymbol(char *dicFile, char *log2phyFile, char *outFile);
};

GENERATESYMBOL::GENERATESYMBOL(){}

GENERATESYMBOL::~GENERATESYMBOL(){}

void GENERATESYMBOL::readFile(char *inFile){
	UTILITY util;
	int lineNum=util.getFileLineCount(inFile);
	FILE *in=util.openFile(inFile, "rt");
	char msg[MAXLEN], *pch, *rtn;
	for(int i=0;i<lineNum;i++){
		rtn=fgets(msg, MAXLEN, in);

		msg[strlen(msg)-1]='\0';
		pch=strtok(msg, "\t ");
		while(pch!=NULL){
			vect.push_back(pch);
			pch=strtok(NULL, "\t ");
		}
	}
	fclose(in);
}

void GENERATESYMBOL::doGenerateSymbol(char *dicFile, char *log2phyFile, char *outFile){
	readFile(dicFile); // dictionary contains [word] [phone 1] [phone 2] ... [phone N]
	readFile(log2phyFile); // SOURCE_LOG2PHY contains [HMM name]

	// obtain unique symbols
	sort(vect.begin(), vect.end());
	vector<string>::iterator it=unique(vect.begin(), vect.end());
	vect.erase(it, vect.end());

	// output
	int symbolNum=(int)vect.size();
	UTILITY util;
	FILE *out=util.openFile(outFile, "wt");
	fprintf(out, "%s\t0\n", EMPTY); // <eps> 0 : <eps> is always indexed 0
	for(int i=0;i<symbolNum;i++){
		if(strcmp(vect[i].c_str(), EMPTY)!=0)
			fprintf(out, "%s\t%d\n", vect[i].c_str(), i+1);
	}
	fclose(out);
}

// Generate symbols
// File 'sym' contains (1) HMM physical name, (2) English words, (3) Chinese words
int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage:%s dic log2phy sym\n", argv[0]);
		exit(1);
	}

	GENERATESYMBOL gs;
	gs.doGenerateSymbol(argv[1], argv[2], argv[3]);

	return 0;
}
