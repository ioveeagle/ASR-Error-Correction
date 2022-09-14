#include <string.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#define EMPTY "<eps>"

using namespace std;
int main(int argc, char **argv){
	if(argc<=3){
		printf("%s src1.sym src2.sym ... sym\n", argv[0]);
		printf("It merges src[1~n].sym to sym\n");
		exit(1);
	}

	string strtmp;
	vector<string> vect;

	for(int i=1;i<argc-1;i++){
		ifstream in(argv[i]);
		if(!in){
			printf("%s not found\n", argv[i]);
			exit(1);
		}
		else{
			while(getline(in, strtmp, '\n'))
			vect.push_back(strtmp.substr(0, strtmp.find('\t')));
		}
	}

	sort(vect.begin(), vect.end());
	vector<string>::iterator it=unique(vect.begin(), vect.end());
	vect.erase(it, vect.end());

	FILE *out=fopen(argv[argc-1], "wt");
	if(out==NULL){
		printf("Error opening file %s\n", argv[argc-1]);
		exit(1);
	}
	else{
		int symNum=vect.size(), count=0;
		fprintf(out, "%s\t%d\n", EMPTY, count);
		count++;
		for(int i=0;i<symNum;i++){
			if(strcmp(vect[i].c_str(), EMPTY)!=0){
				fprintf(out, "%s\t%d\n", vect[i].c_str(), count);
				count++;
			}
		}
	}
	fclose(out);

	return 0;
}
