#include "recog.h"

typedef struct {
	char str[200];
} CMD;

class SCRIPT{
private:
	UTILITY util;
	CMD *header, *remove, *mv, *ext, *cat;
	int platform; // 0: win, 1: unix
	int endOption;// 0: withoutSil 1:withSil 2:@#
	int CICD;
	void genAuxDic(char *fileName);
	void getData(char *fileName);
	void buildGraph(char *fileName);
	void toDecode(char *fileName);
	void toIC(char *fileName);
	void assigneScriptName();
	void checkOption(char *opt1, char *opt2, char *opt3);
	char script1[MAXLEN], script2[MAXLEN], script3[MAXLEN], script4[MAXLEN];
public:
	SCRIPT();
	~SCRIPT();
	void genscript(char  *CICDOption, char *platformOption, char *silOption);
};

SCRIPT::SCRIPT(){
	header = new CMD[2];
	strcpy(header[0].str, "@echo off");
	strcpy(header[1].str, "#!/bin/bash");

	remove = new CMD[2];
	strcpy(remove[0].str, "del /Q");
	strcpy(remove[1].str, "rm");

	mv = new CMD[2];
	strcpy(mv[0].str, "move /Y");
	strcpy(mv[1].str, "mv");

	cat = new CMD[2];
	strcpy(cat[0].str, "type");
	strcpy(cat[1].str, "cat");

	ext = new CMD[2];
	strcpy(ext[0].str, ".bat");
	strcpy(ext[1].str, ".scp");
}

SCRIPT::~SCRIPT(){
	if(header!=NULL)
		delete [] header;
	if(remove!=NULL)
		delete [] remove;
	if(mv!=NULL)
		delete [] mv;
	if(ext!=NULL)
		delete [] ext;
}

void SCRIPT::assigneScriptName(){
	strcpy(script1, "1_getData");
	strcat(script1, ext[platform].str);

	strcpy(script2, "2_buildGraph");
	strcat(script2, ext[platform].str);

	strcpy(script3, "3_toDecode");
	strcat(script3, ext[platform].str);

	strcpy(script4, "toIC");
	strcat(script4, ext[platform].str);
}

void SCRIPT::getData(char *fileName){
	FILE *out=util.openFile(fileName, "wt");
	fprintf(out,"%s\n\n", header[platform].str);

	fprintf(out, "echo ----- make binary macro\n");
	fprintf(out, "macro2tiedstate SOURCE_AM macro.txt\n");
	fprintf(out, "tieda2b macro.txt DEST.macro HMMList\n");	
	fprintf(out, "macrofloat2fix DEST.macro DEST.macro.fix\n");
	fprintf(out, "%s macro.txt HMMList\n", remove[platform].str);
	fprintf(out, "\necho ----- generate symbols\n");
	if(endOption==2){
		fprintf(out, "context2graph SOURCE_LM 0.tmp %d\n", endOption);
		fprintf(out, "graph2symbol 0.tmp 1.tmp\n");
		fprintf(out, "generatesymbol SOURCE_DIC SOURCE_LOG2PHY 2.tmp\n");
		fprintf(out, "mergesymbol 1.tmp 2.tmp sym\n");
		fprintf(out, "%s *.tmp\n", remove[platform].str);
	}
	else
		fprintf(out, "generatesymbol SOURCE_DIC SOURCE_LOG2PHY sym\n");	
	fclose(out);
}

void SCRIPT::buildGraph(char *fileName){
	FILE *out=util.openFile(fileName, "wt");
	fprintf(out,"%s\n\n", header[platform].str);

	fprintf(out, "echo ----- context/LM to word-level graph\n");
	fprintf(out, "context2graph SOURCE_LM 0.tmp %d\n", endOption);
	fprintf(out, "removeredundancy 0.tmp word.G sym 0\n");

	fprintf(out, "\necho ----- word-level graph to CI-phone graph\n");
	fprintf(out, "word2ci word.G 0.tmp SOURCE_DIC sym\n");
	fprintf(out, "removeredundancy 0.tmp ci.G sym 0\n");

	if(CICD==0){
		fprintf(out, "%s ci.G phy.G\n", mv[platform].str);
		fprintf(out, "graph2symbol phy.G phy.S\n");
	}
	else{
		fprintf(out, "\necho ----- CI-phone graph to CD-phone graph\n");
		fprintf(out, "ci2cd ci.G cd.G sym\n");

		fprintf(out, "\necho ----- logical to physical models\n");
		fprintf(out, "graph2symbol cd.G cd.S\n");
		fprintf(out, "log2phy cd.G 0.tmp SOURCE_LOG2PHY cd.S\n");
		fprintf(out, "graph2symbol 0.tmp phy.S\n");
		fprintf(out, "removeredundancy 0.tmp phy.G phy.S 0\n");
	}
	fclose(out);
}

void SCRIPT::toDecode(char *fileName){	
	FILE *out=util.openFile(fileName, "wt");
	fprintf(out,"%s\n\n", header[platform].str);

	fprintf(out, "graphconcat sil.graph phy.G 0.tmp\n");
	fprintf(out, "graph2symbol 0.tmp phy.S\n\n");
	fprintf(out, "partialexpand 0.tmp phy.xp dic.xp phy.S 0\n");
	fprintf(out, "source2dest phy.xp phy.S dic.xp DEST.macro\n");
	fprintf(out, "redirecttable DEST.graph DEST.sym 0.tmp\n");
	fprintf(out, "tablea2b 0.tmp DEST.table\n");
	fprintf(out, "%s *.tmp *.xp *.S *.G sym\n", remove[platform].str);
	fclose(out);
}

void SCRIPT::toIC(char *fileName){
	FILE *out=util.openFile(fileName, "wt");
	fprintf(out,"%s\n\n", header[platform].str);

	fprintf(out,"ICcontext2header SOURCE_LM context.data\n");
	fprintf(out,"ICmacro2header DEST.macro macro.data\n");
	fprintf(out,"ICgraph2header DEST.graph DEST.macro DEST.sym graph.data\n");
	fprintf(out,"ICtable2header DEST.table table.data\n\n");

	fprintf(out, "%s context.data macro.data graph.data table.data > ROM.data\n", cat[platform].str);
	fprintf(out,"ICsave2rom ROM.data\n");

	fprintf(out, "%s *.data\n", remove[platform].str);
	fprintf(out, "echo move ROM.c ROM.h search.h to decoderIC\n");
}

void SCRIPT::checkOption(char *opt1, char *opt2, char *opt3){
	int p1, p2, p3;
	// check value
	p1=atoi(opt1);
	p2=atoi(opt2);
	p3=atoi(opt3);
	if((p1<0)||(p1>1)){
		printf("Error, option1=%d (CI=0, CI=1)\n", p1);
		exit(1);
	}
	if((p2<0)||(p2>1)){
		printf("Error, option2=%d (win=0, unix=1)\n", p2);
		exit(1);
	}
	if((p3<0)||(p3>2)){
		printf("Error, option3=%d, (0=without %s, 1=with %s, 2=with @#)\n", p3, SIL, SIL);
		exit(1);
	}
	// check file existence
	if(p3==2){ // end with @#
		FILE *in=fopen("SOURCE_LM", "rt");
		if(in==NULL){
			printf("SOURCE_LM not exist\n");
			exit(1);
		}
		fclose(in);	
	}

	CICD=p1;
	platform=p2;
	endOption=p3;

	printf("Specified options: ");
	(platform==0)?printf("win/"):printf("unix/");
	(CICD==0)?printf("CI/"):printf("CD/");
	if(endOption==0)
		printf("without %s\n", SIL);
	if(endOption==1)
		printf("with %s\n", SIL);
	if(endOption==2)
		printf("with @#\n");
}

void SCRIPT::genAuxDic(char *fileName){
	FILE *out=util.openFile(fileName, "wt");
	fprintf(out, "sil\tsil\n");
	fprintf(out, "sp\tsp\n");
	fprintf(out, "<s>\tsil\n");
	fprintf(out, "</s>\tsil\n");
	fprintf(out, "<eps>\t<eps>\n");
	fprintf(out, "<F>\t<F>\n");
	if(endOption==2){
		char fileName[100]="SOURCE_LM";
		int lineNum=util.getFileLineCount(fileName);
		for(int i=0;i<lineNum;i++)
			fprintf(out, "@%d\tsil\n", i);
	}
	fclose(out);
}

void SCRIPT::genscript(char *CICDOption, char *platformOption, char *silOption){
	checkOption(CICDOption, platformOption, silOption);

	assigneScriptName();
	getData(script1);
	buildGraph(script2);
	toDecode(script3);
	toIC(script4);

	FILE *f=util.openFile("sil.graph", "wt");
	fprintf(f, "0\t1\tsil\t<eps>\n1\n");
	fclose(f);
	
	f=util.openFile("opt.sil", "wt");
	fprintf(f, "0\t1\tsil\t<eps>\n");
	fprintf(f, "0\t1\tsp\t<eps>\n");
	fprintf(f, "0\t1\t<eps>\t<eps>\n1\n");
	fclose(f);

	char auxFile[100]="SOURCE_DIC.AUX";
	genAuxDic(auxFile);
	printf("\n1) Combine Dics to generate SOURCE_DIC\n");
	printf("2) Run *%s\n", ext[platform].str);
}

int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage:%s CI/CD win/unix end\n", argv[0]);
		printf("CI=0\tCD=1\n\n");
		printf("win=0\tunix=1\n\n");
		printf("end=0\tend without %s\n", SIL);
		printf("end=1\tend with %s\n", SIL);
		printf("end=2\tend with @#\n");
		exit(1);
	}

	SCRIPT script;
	script.genscript(argv[1], argv[2], argv[3]);

	return 0;
}
