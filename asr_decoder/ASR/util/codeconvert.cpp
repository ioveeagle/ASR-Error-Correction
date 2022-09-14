#include "recog.h"

class CODECONVERT{
private:
	char srcDir[200];
	char dstDir[200];
	UTILITY util;
	void getOutStr(char *inStr, char *outStr, char *tabStr, int option);
	void lineProcess(char *inStr, char *outStr);
	void fileConvert(int fileNum, char *tmpFileName, char *srcDir);
public:
	CODECONVERT();
	~CODECONVERT();
	void doConver(char *inSrcDir, char *inDstDir);
};

CODECONVERT::CODECONVERT(){}

CODECONVERT::~CODECONVERT(){}

void CODECONVERT::getOutStr(char *inStr, char *outStr, char *tabStr, int option){
	char msg[MAXLEN];
	int len, s=-1, cnt;
	// parse
	strcpy(msg, inStr);
	len=strlen(msg);
	for(int i=0;i<len;i++){
		if( (msg[i]!=' ')&&(msg[i]!='/')&&(msg[i]!='\t') ){
			s=i;
			break;
		}
	}
	if(s==-1){
		printf("Unable to find / in string %s\n", inStr);
		exit(1);
	}
	// out string
	cnt=0;
	for(int i=s;i<len;i++){
		outStr[cnt]=inStr[i];
		cnt++;
	}
	outStr[cnt]='\0';
	// tab string
	strcpy(tabStr, "");	
	if(msg[s]!='#'){// not #define
		if(option==0){ // add //
			for(int i=0;i<s;i++)
				tabStr[i]='\t';
			tabStr[s]='\0';
			strcat(tabStr, "//");
		}
		else{
			for(int i=0;i<s-2;i++) // discard //
				tabStr[i]='\t';
			tabStr[s-2]='\0';
		}
	}
	else{
		if(option==0)
			strcat(tabStr, "//");
	}
}

void CODECONVERT::lineProcess(char *inStr, char *outStr){
	char msg[MAXLEN], bkStr[MAXLEN], tabStr[MAXLEN], tmp[MAXLEN], *pfloat, *pfixed;
	strcpy(msg, inStr);
	strcpy(bkStr, inStr);

	pfloat=strstr(msg, "@float");
	strcpy(msg, bkStr);
	pfixed=strstr(msg, "@fixed");
	strcpy(msg, bkStr);

	if((pfloat==NULL)&&(pfixed==NULL))
		strcpy(outStr, msg);
	else{		
		if(pfloat!=NULL)
			getOutStr(msg, tmp, tabStr, 0);
		if(pfixed!=NULL)
			getOutStr(msg, tmp, tabStr, 1);			
		sprintf(outStr, "%s%s", tabStr, tmp);
	}
}

void CODECONVERT::fileConvert(int fileNum, char *tmpFileName, char *srcDir){	
	if(fileNum==0){
		printf("files not found in %s, assign complete path\n", srcDir);
		exit(1);
	}

	FILE *in, *f, *g;
	char fileName[MAXLEN], srcFile[MAXLEN], dstFile[MAXLEN], msg[MAXLEN], outStr[MAXLEN];
	int lineNum;

	in=util.openFile(tmpFileName, "rt");
	for(int i=0;i<fileNum;i++){
		fgets(fileName, MAXLEN, in);
		fileName[strlen(fileName)-1]='\0';

		strcpy(srcFile, srcDir);
		strcat(srcFile, "\\");
		strcat(srcFile, fileName);
		strcpy(dstFile, dstDir);
		strcat(dstFile, "\\");
		strcat(dstFile, fileName);

		lineNum=util.getFileLineCount(srcFile);
		f=util.openFile(srcFile, "rt");
		g=util.openFile(dstFile, "wt");
		for(int j=0;j<lineNum;j++){
			fgets(msg, MAXLEN, f);
			msg[strlen(msg)-1]='\0';

			lineProcess(msg, outStr);
			fprintf(g, "%s\n", outStr);
		}
		fclose(f);
		fclose(g);
	}
	fclose(in);
}

void CODECONVERT::doConver(char *inSrcDir, char *inDstDir){
	strcpy(srcDir, inSrcDir);
	strcpy(dstDir, inDstDir);

	char cmd[MAXLEN], tmpFile[MAXLEN];
	int fileNum;
	strcpy(tmpFile, "list.tmp");

  // process .h files
	sprintf(cmd, "dir /b %s\\*.h > %s\n", srcDir, tmpFile);
	system(cmd);
	fileNum=util.getFileLineCount(tmpFile);
	fileConvert(fileNum, tmpFile, srcDir);

  // process .cpp files
	sprintf(cmd, "dir /b %s\\*.cpp > %s\n", srcDir, tmpFile);
	system(cmd);
	fileNum=util.getFileLineCount(tmpFile);
	fileConvert(fileNum, tmpFile, srcDir);

	sprintf(cmd, "del /Q %s\n", tmpFile);
	system(cmd);
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s srcDir dstDir\n", argv[0]);
		printf("Convert decoder.float to decoder.fixed\n");
		exit(1);
	}

	CODECONVERT cc;
	cc.doConver(argv[1], argv[2]);

	return 0;
}
