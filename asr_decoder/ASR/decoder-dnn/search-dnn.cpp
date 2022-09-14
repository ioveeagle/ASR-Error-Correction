#include "recog-dnn.h"
#if defined(MOBILE_FRAMESKIP)
#define FRAMESKIP_LEN 2
#endif
void SEARCH::getActiveToken(){
	// histogram pruning: assigning different beamSize
	/*
	if(frame==30)
		beamSize=(int)(beamSize*0.95);
	if(frame==50)
		beamSize=(int)(beamSize*0.9);
	if(frame==60)
		beamSize=(int)(beamSize*0.9);
	if(frame==70)
		beamSize=(int)(beamSize*0.9);
	if(frame==90)
		beamSize=(int)(beamSize*0.9);
	if(frame==150)
		beamSize=(int)(beamSize*0.9);
	if(frame==200)
		beamSize=(int)(beamSize*0.7);
	if(frame==300)
		beamSize=(int)(beamSize*0.8);
	
	if(frame==100)
		beamSize=(int)(beamSize*1.2);
	if(frame==200)
		beamSize=(int)(beamSize*1.2);
	*/
	beamSize=beamSize;
	

	// token number is at least 100
	(beamSize>100)?tokenNum=beamSize:tokenNum=100;

	//if(tmpTokenNum>maxTokenNum) maxTokenNum = tmpTokenNum; 

	if(tmpTokenNum>beamSize){		
		U.pseudoSort(p2arena, 0, tmpTokenNum-1, beamSize); // only sort out Top-beamSize candidates
		//U.quickSort(p2arena, 0, tmpTokenNum-1);
		tokenNum=tmpTokenNum=beamSize;
	}
	else
		tokenNum=tmpTokenNum;

}
/*
double SEARCH::mixLogSum(double x,double y){
	double MIN_LOG_EXP=-23.02585; // Min usable log exp, MIN_LOG_EXP = -log(-LZERO)
	double LZERO=-1.0E10; // value of log(0)
	double LSMALL=-0.5E10; // log(x) < LSAMLL is set LZERO
	double diff, temp; 
	if (x<y) {
		temp=x; x=y; y=temp;
	}
	diff=y-x;
	if (diff<MIN_LOG_EXP)
		return (x<LSMALL)?LZERO:x;
	else{
		#if defined(ASR_SSE2)
		__m128d diffTemp={diff, 0};
		return x+mixLogSumLookupTable[_mm_cvtsd_si32(_mm_mul_sd(diffTemp, mixLogSumLookupScale))];
		#elif defined(ASR_AVX)
		__m256d diffTemp={diff, 0, 0, 0};
		return x+mixLogSumLookupTable[_mm_cvtsi128_si32((_mm256_cvtpd_epi32(_mm256_mul_pd(diffTemp, mixLogSumLookupScale))))];
		#else
		double diffTemp=diff;
		return x+mixLogSumLookupTable[(int)(diffTemp*mixLogSumLookupScale)];
		#endif
	}
}

SCORETYPE SEARCH::streamScore(int tiedStateIdx){
	
	#ifndef ASR_FIXED
	float mixtureScore;//@float
	int mixtureNum=M.p2ts[tiedStateIdx]->mixtureNum, loopNum;//@float
	double score=-1.0E10;//@float
	#endif

  	#ifdef ASR_SSE2
  	//---------- floating point
	loopNum=M.streamWidth>>2;
  	ALIGNTYPE float ms[4];//@float
    
  	__m128 *f = (__m128*) p2feaVec;//@float
  	__m128 *f0 = f;//@float
  	__m128 *m = (__m128*) M.p2ts[tiedStateIdx]->mean;//@float
  	__m128 *v = (__m128*) M.p2ts[tiedStateIdx]->variance;//@float
  	__m128 d1, d2, d3, sum;//@float
  
  	for(int i=0;i<mixtureNum;i++){//@float
  		sum=_mm_setzero_ps();//@float
  		for(int j=0;j<loopNum;j++){//@float
  			d1=_mm_sub_ps(*(f++), *(m++));//@float, (f-m): differece between f and the mean
  			d2=_mm_mul_ps(d1, d1);// a=diff*diff //@float
  			d3=_mm_mul_ps(d2, *(v++));// b=a*variance //@float
  			sum=_mm_add_ps(sum, d3);//@float
  		}//@float
  		_mm_store_ps(ms, sum);//@float
  		f=f0;//@float
  		
  		mixtureScore=-(ms[0]+ms[1]+ms[2]+ms[3]+M.p2ts[tiedStateIdx]->gconst[i])*0.5f;//@float
  		score=mixLogSum(score, M.p2ts[tiedStateIdx]->mixtureWeight[i]+mixtureScore);//@float
  	}//@float
	#elif defined(ASR_AVX)
	loopNum=M.streamWidth>>3;
	ALIGNTYPE float ms[8];
	
  	__m256 *f = (__m256*) p2feaVec;//@float
  	__m256 *f0 = f;//@float
  	__m256 *m = (__m256*) M.p2ts[tiedStateIdx]->mean;//@float
  	__m256 *v = (__m256*) M.p2ts[tiedStateIdx]->variance;//@float	
  	__m256 d1, d2, d3, sum;//@float

	
  	for(int i=0;i<mixtureNum;i++){//@float
		//sum=_mm256_setzero_ps();//@float
		sum=_mm256_xor_ps(sum, sum);
		
  		for(int j=0;j<loopNum;j++){//@float
  			d1=_mm256_sub_ps(*(f++), *(m++));//@float, (f-m): differece between f and the mean
  			d2=_mm256_mul_ps(d1, d1);// a=diff*diff //@float
  			d3=_mm256_mul_ps(d2, *(v++));// b=a*variance //@float
  			sum=_mm256_add_ps(sum, d3);//@float
  		}//@float		
		

  		_mm256_store_ps(ms, sum);//@float
  		f=f0;//@float
		

		ms[0]+=ms[1];
		ms[2]+=ms[3];
		ms[4]+=ms[5];
		ms[6]+=ms[7];
		ms[0]+=ms[2];
		ms[4]+=ms[6];
		ms[0]+=ms[4];
		mixtureScore=-( ms[0] +M.p2ts[tiedStateIdx]->gconst[i])*0.5f;//@float
		//mixtureScore=-( (ms[0]+ms[1]+ms[2]+ms[3]+ms[4]+ms[5]+ms[6]+ms[7])+M.p2ts[tiedStateIdx]->gconst[i])/2;//@float
  		score=mixLogSum(score, M.p2ts[tiedStateIdx]->mixtureWeight[i]+mixtureScore);//@float
  	}//@float	
	#elif defined(ASR_FIXED)
  	//---------- fixed point
  	int score=-2147483647, mixtureScore;//@fixed
  	int mixtureNum=M.p2ts[tiedStateIdx]->mixtureNum;//@fixed
  	SCORETYPE diff, *p2m=M.p2ts[tiedStateIdx]->mean, *p2v=M.p2ts[tiedStateIdx]->variance;//@fixed
  
  	for(int i=0;i<mixtureNum;i++){//@fixed
  		mixtureScore=0;//@fixed
  		diff=0;//@fixed
  		for(int j=0;j<M.streamWidth;j++){//@fixed
  			diff=p2feaVec[j]-p2m[j];//@fixed
  			mixtureScore+=((diff*p2v[j])>>17)*diff>>1;//@fixed
  		}//@fixed
  
  		p2m+=M.streamWidth;//@fixed
  		p2v+=M.streamWidth;//@fixed
  		mixtureScore=mixtureScore>>2;//@fixed
  		mixtureScore=-((mixtureScore+M.p2ts[tiedStateIdx]->gconst[i])>>1);//@fixed
  
  		if(mixtureScore>score)//@fixed
  			score=mixtureScore;//@fixed
  	}//@fixed
	#else
	// ASR_GENERIC
  	//---------- generic floating point manipulation
  	float diff, *p2m=M.p2ts[tiedStateIdx]->mean, *p2v=M.p2ts[tiedStateIdx]->variance;//@generic
  
  	for(int i=0;i<mixtureNum;i++){//@generic
  		mixtureScore=0;//@generic
  		diff=0;//@generic
  		for(int j=0;j<M.streamWidth;j++){//@generic
  			diff=p2feaVec[j]-p2m[j];//@generic
  			mixtureScore+=(diff*diff)*p2v[j];//@generic
  			//mixtureScore+=((diff*p2v[j])>>17)*diff>>1;//@generic			
  		}//@generic
  
  		p2m+=M.streamWidth;//@generic
  		p2v+=M.streamWidth;//@generic
  		//mixtureScore=mixtureScore/4;//@generic
  		//mixtureScore=mixtureScore>>2;//@generic
  		mixtureScore=-((mixtureScore+M.p2ts[tiedStateIdx]->gconst[i])/2);//@generic
  		//mixtureScore=-((mixtureScore+M.p2ts[tsIdx]->gconst[i])>>1);//@generic
  
  		score=mixLogSum(score, M.p2ts[tiedStateIdx]->mixtureWeight[i]+mixtureScore);//@generic
  		//if(mixtureScore>score)//@generic
  		//score=mixtureScore;//@generic
  	}//@generic	
	#endif
	
	return (SCORETYPE)score;
}
*/

float SEARCH::transitionProb(int MMFIdx, int state, int calCase){
	// calCase: 1, 2, 3=> self state, next state, entering a new phone
	int matrixDim, pos;
	float tmProb;
	if(calCase==3)
		tmProb=0;
	else{
		matrixDim=D.p2MMF[MMFIdx]->stateNum;
		// transition matrix is of size (stateNum+2)x(stateNum+2)
		// xxxxx/xxxxx/xxxxx/xxxxx/xxxxx
		// self state: 
		(calCase==1)?pos=matrixDim*(state+1)+(state+1):pos=matrixDim*(state+1)+(state+1)+1;
		//if( calCase==2) printf("I: state: %d pos: %d (%s)\n", state, pos, __FUNCTION__); // SEE
		tmProb=D.p2tm[D.p2MMF[MMFIdx]->tiedMatrixIndex]->transProb[pos];
	}
	return tmProb;
}

/*
SCORETYPE SEARCH::stateScore(int tiedStateIdx){
	if(cacheScore[tiedStateIdx]==0)
		cacheScore[tiedStateIdx]=streamScore(tiedStateIdx);
	return cacheScore[tiedStateIdx];
}
*/

SCORETYPE SEARCH::getTokenScore(int node, int branch, int phonePos, int state, int calCase, SCORETYPE tokenScore, SCORETYPE LMscore){
	int tsIdx, MMFIdx;
	SCORETYPE score;
	MMFIdx=obtainModel(node, branch, phonePos); // retrieve model index according to node's branch

	tsIdx=D.p2MMF[MMFIdx]->tiedStateIndex[state];
	//printf("[TSIDX] %d %d %d\n", feaFileIndex, frame, tsIdx);
	//score=stateScore(tsIdx)+tokenScore+LMscore;
	#if defined(MOBILE_FRAMESKIP)
	if( frame>0 ){
		int cond=frame%FRAMESKIP_LEN;
		if( cond ==1 ){
			score=D.frameScore[frame][tsIdx]+tokenScore+LMscore;
		}else{
			int lookframe=frame - (frame-1)%FRAMESKIP_LEN;
			score=D.frameScore[lookframe][tsIdx]+tokenScore+LMscore;
		}
	}else{
		score=D.frameScore[frame][tsIdx]+tokenScore+LMscore;
        }
	#else
	score=D.frameScore[frame][tsIdx]+tokenScore+LMscore;
	#endif

	if( 0 && D.frameScore[frame][tsIdx] > 0 ){
		printf("tsIdx:%d state:%d node:%d branch:%d\n", tsIdx, state, node, branch);
		printf("%5d %10d s:%10.6f am:%10.6f lm:%10.6f prevs:%10.6f\n", frame, node, score, D.frameScore[frame][tsIdx], LMscore, tokenScore);
	}

	#ifdef ASR_FIXED
	score+=0; //@fixed
	#else
	//if( calCase ==2 ) printf("I: state: %d calCase: %d (%s)\n", state, calCase, __FUNCTION__); // SEE
	//if( calCase == 2 )
	//	score+=transitionProb(MMFIdx, state-1, calCase);
	//else
		score+=transitionProb(MMFIdx, state, calCase);//@float	
	#endif	

	return score;
}
// determine which token can survive pass through the given WFST node
void SEARCH::compete(int node, SCORETYPE score, int competeTokenIdx){
	int cupHolderTokenIdx=G.p2N[node]->token;
	if(G.p2N[node]->feaFile==feaFileIndex){
		if(G.p2N[node]->frame==frame){
			if(p2arena[cupHolderTokenIdx]->score<score)
				G.p2N[node]->token=competeTokenIdx;
		}
		else{
			G.p2N[node]->frame=frame;
			G.p2N[node]->token=competeTokenIdx;
		}
	}
	else{
		G.p2N[node]->feaFile=feaFileIndex;
		G.p2N[node]->frame=frame;
		G.p2N[node]->token=competeTokenIdx;
	}
}

void SEARCH::selfState(){
	int aN=tmpTokenNum;
	SCORETYPE score;
		
	for(int i=0;i<tokenNum;i++){
		score=getTokenScore(p2arena[i]->node, p2arena[i]->branch, p2arena[i]->phonePos, p2arena[i]->state, 1, p2arena[i]->score, 0);
		#ifdef USE_LATTICE
		amScore = score - p2arena[i]->score;
		lmScore = 0.0;
		//p2arena[i]->amScore += (score - p2arena[i]->score);
		//printf("\tfr:%d %d %f %f %f %s\n", frame, i, score, p2arena[i]->amScore, p2arena[i]->lmScore, __FUNCTION__);
		#endif
		save2arena(0, i, aN, p2arena[i]->node, p2arena[i]->branch, p2arena[i]->state, p2arena[i]->hist, -2, score, p2arena[i]->phonePos);
	}
	tmpTokenNum=aN;
}

void SEARCH::updatestateCompete(int token){
	int dst;
	int pointer; // @20120723
	pointer = arc2stateCompetePointer[G.p2N[p2arena[token]->node]->arcPos+p2arena[token]->branch]; // @20120723

	// p2arena[token]->node   : the token comes from 'node'
	// p2arena[token]->branch : the token enter the 'branch' of 'node'
	//if(arc2stateCompete[G.p2N[p2arena[token]->node]->arcPos+p2arena[token]->branch]==-1){//personne est la
	if(arc2stateCompete[pointer+p2arena[token]->phonePos]==-1){ // @20120723
		dst=stateCompeteCounter;
		arc2stateCompete[pointer+p2arena[token]->phonePos] = dst; // @20120723
		//arc2stateCompete[G.p2N[p2arena[token]->node]->arcPos+p2arena[token]->branch]=dst;
		stateCompete[dst*MAX_STATE_NUM+p2arena[token]->state]=token;
		stateCompeteCounter++;
	}
	else{
		
		dst=arc2stateCompete[pointer+p2arena[token]->phonePos]; // @20120723
		//dst=arc2stateCompete[G.p2N[p2arena[token]->node]->arcPos+p2arena[token]->branch];		
		stateCompete[dst*MAX_STATE_NUM+p2arena[token]->state]=token;
	}
}

void SEARCH::nextState(){
	competeAtNode();

	SCORETYPE score;
	int aN=tokenNum, MMFIdx;
	for(int i=0;i<tokenNum;i++){

		MMFIdx=obtainModel(p2arena[i]->node, p2arena[i]->branch, p2arena[i]->phonePos);

		if(p2arena[i]->state==D.p2MMF[MMFIdx]->stateNum-3){ // exit HMM, go to a new WFST node
			int arcPos=G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch;
			int inLabel=G.arc[arcPos].in;			
			
			if(inLabel>=0) // D!=#, typical route
				aN=reDirection(G.arc[arcPos].endNode, aN, i);
			else{ // D=#
				if(p2arena[i]->phonePos+1==dic[-(inLabel+DIC_OFFSET)].phoneNum)
					aN=reDirection(G.arc[arcPos].endNode, aN, i);
				else{					
					score=getTokenScore(p2arena[i]->node, p2arena[i]->branch, p2arena[i]->phonePos, 0, 3, p2arena[i]->score, 0);
					#ifdef USE_LATTICE
					amScore = score-p2arena[i]->score;
					lmScore = 0.0;
					#endif
					save2arena(1, i, aN, p2arena[i]->node, p2arena[i]->branch, 0, p2arena[i]->hist, -2, score, p2arena[i]->phonePos+1);
				}
			}
		}
		else{ // go to next state within an HMM
			//printf("I: state: %d (%s)\n", p2arena[i]->state, __FUNCTION__); // SEE
			score=getTokenScore(p2arena[i]->node, p2arena[i]->branch, p2arena[i]->phonePos, p2arena[i]->state+1, 2, p2arena[i]->score, 0);
			//score=getTokenScore(p2arena[i]->node, p2arena[i]->branch, p2arena[i]->phonePos, p2arena[i]->state, 2, p2arena[i]->score, 0);
			#ifdef USE_LATTICE
			amScore = score-p2arena[i]->score;
			lmScore = 0.0;
			//p2arena[i]->amScore += (score - p2arena[i]->score);
			//printf("\tfr:%d %d %f %f %f %s\n", frame, i, score, p2arena[i]->amScore, p2arena[i]->lmScore, __FUNCTION__);
			#endif
			save2arena(1, i, aN, p2arena[i]->node, p2arena[i]->branch, p2arena[i]->state+1, p2arena[i]->hist, -2, score, p2arena[i]->phonePos);
		}
	}
	tmpTokenNum=aN;
}

void SEARCH::save2arena(int type, int token, int &save2Pos, int node, int branch, int state, int hist, int reDirectPath, SCORETYPE score, int phonePos){
	int stateCompetePos, defender;
	bool win;
	int pointer; // @20120723

	#ifdef USE_LATTICE
	tmpAmScore = p2arena[token]->amScore;
	tmpLmScore = p2arena[token]->lmScore;
	#endif

	// compare score
	win=true;
	defender=-1;

	pointer = arc2stateCompetePointer[G.p2N[node]->arcPos+branch]; // @20120723
	stateCompetePos=arc2stateCompete[pointer+phonePos]; // @20120723
	//stateCompetePos=arc2stateCompete[G.p2N[node]->arcPos+branch];

	if(stateCompetePos!=-1){
		// TODO: didn't take D=# into consideration...
		defender=stateCompete[stateCompetePos*MAX_STATE_NUM+state];
		if(defender!=-1){ // state has its own score already
			if(p2arena[defender]->score>score) // defender token wins!
				win=false;
		}
	}

	// win or the first one reaching here
	if(type==0){// self state
		if(win==true){
			updateToken(token, node, branch, state, hist, reDirectPath, score, phonePos);
			if(defender!=-1){
				*p2arena[defender]=*p2arena[save2Pos-1];
				updatestateCompete(defender);
				save2Pos--;
			}
		}
		else{
			*p2arena[token]=*p2arena[save2Pos-1];
			updatestateCompete(token);
			save2Pos--;
		}
	}
	else{// next state, type==1
		if(win==true){ // does the incoming token defeat the defender token?
			if(defender!=-1) // defeat defender token
				updateToken(defender, node, branch, state, hist, reDirectPath, score, phonePos);
			else{ // no defender token in the state
				updateToken(save2Pos, node, branch, state, hist, reDirectPath, score, phonePos);
				updatestateCompete(save2Pos);
				save2Pos++; // aN, the value returned by move2Transition()
			}
		}
	}
}

int SEARCH::move2Transition(int reDirectedNode, int endNode, int aN, int r, int tokenIdx){
	SCORETYPE redirectedScore, LMscore, score;
	// redirected probability
	// if it's a redirected path, use 'LPProb[LTendNodeArrayPosition[endNode]+r]';
	// otherwise, there is no redirect path, hence no redirect score.
	(r>-1)?redirectedScore=LTProb[LTendNodeArrayPosition[endNode]+r]:redirectedScore=0;
  // In the initialize(), 'token' in 'endNode' and its 'redirectedNode' are initialized to 'tokenIndex'
	if(G.p2N[reDirectedNode]->token==tokenIdx){// the token allowed to pass through the WFST node, which was determined in competeAtNode()
		for(int i=0;i<G.p2N[reDirectedNode]->branchNum;i++){
			if(G.arc[G.p2N[reDirectedNode]->arcPos+i].in!=-1){ // branches of 'reDirectedNode' are not <eps>								
				#ifdef ASR_FIXED
				LMscore=redirectedScore+(int)G.arc[G.p2N[reDirectedNode]->arcPos+i].weight; //@fixed				
				#else
				LMscore=redirectedScore+G.arc[G.p2N[reDirectedNode]->arcPos+i].weight; //@float		
				if( G.arc[G.p2N[reDirectedNode]->arcPos+i].weight != 0.0f ) LMscore -= 0.0; // phone insertion penalty	
				#endif

				score=getTokenScore(reDirectedNode, i, 0, 0, 3, p2arena[tokenIdx]->score, LMscore);
				#ifdef USE_LATTICE
				amScore = score - p2arena[tokenIdx]->score - LMscore;
				lmScore = LMscore;
				//if(i==0){
				//	p2arena[tokenIdx]->lmScore += LMscore;
				//	p2arena[tokenIdx]->amScore += (score - p2arena[tokenIdx]->score - LMscore);
				//}
				//printf("\tfr:%d %d %f %f %f %s\n", frame, tokenIdx, score, p2arena[tokenIdx]->amScore, p2arena[tokenIdx]->lmScore, __FUNCTION__);
				//if(LMscore>0.0f){
				//	printf("\tt:%d LM:%f reS:%f arcS:%f\n", tokenIdx, LMscore, redirectedScore, LMscore-redirectedScore);
				//	printf("\t\tlmS:%f amS:%f S:%f\n", p2arena[tokenIdx]->lmScore, p2arena[tokenIdx]->amScore, score);
				//}
				#endif
				save2arena(1, tokenIdx, aN, reDirectedNode, i, 0, p2arena[tokenIdx]->hist, r, score, 0);
			}
		}
	}
	return aN;
}

void SEARCH::updateToken(int token, int node, int branch, int state, int hist, int reDirectPath, SCORETYPE score, int phonePos){
	p2arena[token]->node=node;
	p2arena[token]->branch=branch;
	p2arena[token]->state=state;
	p2arena[token]->hist=hist;
	p2arena[token]->reDirectPath=reDirectPath;
	p2arena[token]->score=score;
	p2arena[token]->phonePos=phonePos;

	#ifdef USE_LATTICE
	p2arena[token]->lmScore = tmpLmScore;
	p2arena[token]->amScore = tmpAmScore;

	p2arena[token]->lmScore += lmScore;
	p2arena[token]->amScore += amScore;
	#endif
	
}

void SEARCH::competeAtNode(){ // Token competition is performed
	int endNode, reDirectedNode, MMFIdx;
	SCORETYPE score;

	for(int i=0;i<tokenNum;i++){
		MMFIdx=obtainModel(p2arena[i]->node, p2arena[i]->branch, p2arena[i]->phonePos);

		int arcPos=G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch; // @20120725
		int inLabel=G.arc[arcPos].in; // @20120725
		bool cond=true; // @20120725
		if( inLabel<-1 ) // @20120725
			((p2arena[i]->phonePos+1)==dic[-(inLabel+DIC_OFFSET)].phoneNum)?cond=true:cond=false; // @20120725

		// leave the last state of current phone to the next WFST node
		//if(p2arena[i]->state+1==M.p2MMF[MMFIdx]->stateNum-2){
		if( (p2arena[i]->state+1==D.p2MMF[MMFIdx]->stateNum-2)&&(cond==true)){  // @20120725
			endNode=G.arc[G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch].endNode;

			//p2arena[i]->score+=transitionProb(MMFIdx, p2arena[i]->state, 2);//@float	
			//if(LTendNodeNum[endNode]>0){
			if((LTendNodeNum[endNode]>0)&&(cond==true)){ // @20120725
				for(int j=0;j<LTendNodeNum[endNode];j++){
					reDirectedNode=LTendNodeArray[LTendNodeArrayPosition[endNode]+j];
					score=p2arena[i]->score+LTProb[LTendNodeArrayPosition[endNode]+j];
					compete(reDirectedNode, score, i);
				}
			}
			compete(endNode, p2arena[i]->score, i); //@20130108 p2arena[i]->score -> p2->score+score
		}
	}
}

int SEARCH::reDirection(int endNode, int aN, int tokenIdx){
	int reDirectEndNode;
	for(int r=0;r<LTendNodeNum[endNode];r++){ // for each redirect path
		// visiting node is 'endNode'; the next node to visit is 'reDirectEndNode'
		reDirectEndNode=LTendNodeArray[LTendNodeArrayPosition[endNode]+r];
		// visit the transition between 'endNode' and 'reDirectEndNode'
		aN=move2Transition(reDirectEndNode, endNode, aN, r, tokenIdx);
	}
	aN=move2Transition(endNode, endNode, aN, -1, tokenIdx);
	return aN;
}

void SEARCH::accessHistory(){
	int histIdx;
	for(int i=0;i<tokenNum;i++){
		histIdx=p2arena[i]->hist;
		if(hist[histIdx].frame!=frame){
			hist[histIdx].frame=frame;
			hist[histIdx].accessed=1;
		}
		else
			hist[histIdx].accessed++;
	}
}
void SEARCH::updateHistoryFramewise(){
	int histDest=0, histSrc, loopStart=0;
	int currentOutLabel, prevOutLabel;
	int currentInLabel; // banco@20120629
	float currentWeight; // banco@20120629
	accessHistory();

	for(int i=0;i<tokenNum;i++){		
		if(p2arena[i]->reDirectPath>=-2){// see reDirection();
			histSrc=p2arena[i]->hist;
			currentInLabel=G.arc[G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch].in; // banco@20120629
			if( strcmp("<eps>", D.p2MMF[currentInLabel]->str)!=0 ){
				if(hist[histSrc].accessed>1){
					for(int j=loopStart;j<tmpTokenNum;j++){
						if(hist[j].frame!=frame){
							histDest=j;
							loopStart=j+1;
							break;
						}
					}
					memcpy(hist[histDest].arc, hist[histSrc].arc, sizeof(int)*hist[histSrc].pathNum);
					//memcpy(hist[histDest].frameBoundary, hist[histSrc].frameBoundary, sizeof(int)*hist[histSrc].pathNum);
					#ifdef USE_LATTICE
					memcpy(hist[histDest].lmScore, hist[histSrc].lmScore, sizeof(SCORETYPE)*hist[histSrc].pathNum);
					memcpy(hist[histDest].amScore, hist[histSrc].amScore, sizeof(SCORETYPE)*hist[histSrc].pathNum);
					#endif
					hist[histSrc].accessed--;
					hist[histDest].pathNum=hist[histSrc].pathNum;
					p2arena[i]->hist=histDest;
				}
				else
					histDest=histSrc;// append to itself

				// append the path and frameBoundary
				hist[histDest].arc[hist[histDest].pathNum]=G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch;
				//hist[histDest].frameBoundary[hist[histDest].pathNum]=frame;
				hist[histDest].pathNum++;
				if(hist[histDest].pathNum>=MAX_HISTORY_LEN){
					printf("Error: pathNum exceeds MAX_HISTORY_LEN %d, frame %d, token %d\n", MAX_HISTORY_LEN, frame, i);
					exit(1);
				}
			}
		}
	}
}

void SEARCH::updateHistory(){
	int histDest=0, histSrc, loopStart=0;
	int currentOutLabel, prevOutLabel;
	int currentInLabel; // banco@20120629
	float currentWeight; // banco@20120629
	accessHistory();

	/*
	float currentMaxScore = p2arena[0]->score;
	int currentMaxScoreToken = 0;
	float currentBeamWidth = 100.0;
	for(int i=1; i<tokenNum; i++){
		if( p2arena[i]->score > currentMaxScore ){
			currentMaxScore = p2arena[i]->score;
			currentMaxScoreToken = i;
		}
	}
	printf("MaxScoreToken: %d %f\n", currentMaxScoreToken, currentMaxScore);
	*/

	for(int i=0;i<tokenNum;i++){
		if(p2arena[i]->reDirectPath>=-1){// see reDirection();
		//if(p2arena[i]->reDirectPath==-2 && p2arena[i]->state==2){
			histSrc=p2arena[i]->hist;
			currentOutLabel=G.arc[G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch].out;
			currentInLabel=G.arc[G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch].in; // banco@20120629
			currentWeight=G.arc[G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch].weight; // banco@20120629

			(hist[histSrc].pathNum>0)?prevOutLabel=G.arc[hist[histSrc].arc[hist[histSrc].pathNum-1]].out:prevOutLabel=epsIndex;

			/*
			if( p2arena[i]->score >= currentMaxScore - currentBeamWidth ){
				printf("%d\t%d\t%s\t%s\n",frame,i, D.p2MMF[currentInLabel]->str, G.symbolList[currentOutLabel].str);

				if( i == currentMaxScoreToken ){
					printf("Best:");
					for( int j=0; j<hist[histSrc].pathNum; j++ ){
						int pos = hist[histSrc].arc[j];
						printf(" %s %d", G.symbolList[G.arc[pos].out].str, hist[histSrc].frameBoundary[j]);					
					}
					printf("\n");
				}	
			}
			*/

			if( (currentOutLabel!=epsIndex )
			   //||((prevOutLabel!=epsIndex)&&(currentOutLabel==epsIndex))
			)
			{// the 2nd is needed only when frameboundary is used				
				// find the destination to copy to
				if(hist[histSrc].accessed>1){
					for(int j=loopStart;j<tmpTokenNum;j++){
						if(hist[j].frame!=frame){
							histDest=j;
							loopStart=j+1;
							break;
						}
					}
					memcpy(hist[histDest].arc, hist[histSrc].arc, sizeof(int)*hist[histSrc].pathNum);
					memcpy(hist[histDest].frameBoundary, hist[histSrc].frameBoundary, sizeof(int)*hist[histSrc].pathNum);
					#ifdef USE_LATTICE
					memcpy(hist[histDest].lmScore, hist[histSrc].lmScore, sizeof(SCORETYPE)*hist[histSrc].pathNum);
					memcpy(hist[histDest].amScore, hist[histSrc].amScore, sizeof(SCORETYPE)*hist[histSrc].pathNum);
					#endif
					hist[histSrc].accessed--;
					hist[histDest].pathNum=hist[histSrc].pathNum;
					p2arena[i]->hist=histDest;
				}
				else
					histDest=histSrc;// append to itself

				// append the path and frameBoundary
				hist[histDest].arc[hist[histDest].pathNum]=G.p2N[p2arena[i]->node]->arcPos+p2arena[i]->branch;
				hist[histDest].frameBoundary[hist[histDest].pathNum]=frame;
					
				#ifdef USE_LATTICE
				hist[histDest].lmScore[hist[histDest].pathNum] = p2arena[i]->lmScore;
				hist[histDest].amScore[hist[histDest].pathNum] = p2arena[i]->amScore;

				//printf("fr:%d histDest:%d arc:%d label:%s score:%f amS:%f lmS:%f\n", frame, histDest, hist[histDest].arc[hist[histDest].pathNum], G.symbolList[currentOutLabel].str, p2arena[i]->score, p2arena[i]->amScore, p2arena[i]->lmScore);

				#endif
				// increase the pathNum
				hist[histDest].pathNum++;
				if(hist[histDest].pathNum>=MAX_HISTORY_LEN){
					printf("Error: pathNum exceeds MAX_HISTORY_LEN %d, frame %d, token %d\n", MAX_HISTORY_LEN, frame, i);
					exit(1);
				}


			}
		}
	}
}

void SEARCH::initialize(){
	resetGraphInfo(0);

	frame=0;
	tmpTokenNum=0;

	// competeAtNode()
	for(int r=0;r<LTendNodeNum[0];r++) // Lookup Table
		G.p2N[LTendNodeArray[LTendNodeArrayPosition[0]+r]]->token=0;
	G.p2N[0]->token=0;
	// assign initial values to token:0
	p2arena[0]->node=0;
	p2arena[0]->branch=0;
	p2arena[0]->phonePos=0;
	p2arena[0]->state=0;
	p2arena[0]->reDirectPath=-2;
	p2arena[0]->score=0;
	p2arena[0]->hist=0;

	#ifdef USE_LATTICE
	p2arena[0]->lmScore = 0;
	p2arena[0]->amScore = 0;
	#endif
	// evaluate 1st frame (index 0) here
	tmpTokenNum=reDirection(0, 0, 0);

	resetGraphInfo(1);
	getActiveToken();
	#ifdef USE_FRAMEWISE_OUTPUT
	updateHistoryFramewise();
	#else
	updateHistory();
	#endif
}

void SEARCH::doDecode(int keepBeamSize, int frameNum, SCORETYPE *feaVec, int fileIndex){

	beamSize=keepBeamSize;
	feaFileIndex=fileIndex;
	p2feaVec=feaVec;


	// the first frame (frame=0) is evaluated here
	#if defined(ASR_SSE2) || defined(ASR_AVX) || defined(ASR_NEON)
	D.propagateSIMD(feaVec, 0, D.sizeBlock, D.streamWidth);
	#else
	D.propagate(feaVec, 0, D.sizeBlock, D.streamWidth);
	#endif
	initialize();

	#ifdef USE_LATTICE
	/*
	char outputArena[MAXLEN];
	sprintf( outputArena, "LatticeInfo_%d.txt", fileIndex);		
	U.showLattice(p2arena, hist, G, M, outputArena, tokenNum, frame, 1);
	*/
	#endif

	for(frame=1;frame<frameNum;frame++){
		p2feaVec=feaVec+frame*D.streamWidth;

		#if defined(MOBILE_FRAMESKIP)
		if( frame%FRAMESKIP_LEN==1 ){
			#if defined(ASR_SSE2) || defined(ASR_AVX) || defined(ASR_NEON)
			D.propagateSIMD(feaVec, frame, D.sizeBlock, D.streamWidth);
			#else
			D.propagate(feaVec, frame, D.sizeBlock, D.streamWidth);
			#endif
		}
		#else

			#if defined(ASR_SSE2) || defined(ASR_AVX) || defined(ASR_NEON)
			D.propagateSIMD(feaVec, frame, D.sizeBlock, D.streamWidth);
			#else
			D.propagate(feaVec, frame, D.sizeBlock, D.streamWidth);
			#endif
		#endif

		nextState();
		selfState();


		resetGraphInfo(1);
		getActiveToken();
		#ifdef USE_FRAMEWISE_OUTPUT
		updateHistoryFramewise();
		#else
		updateHistory();
		#endif

		#ifdef USE_LATTICE
		//U.showLattice(p2arena, hist, G, M, outputArena, tokenNum, frame, 1);
		#endif

		//char msg[100];
		//sprintf(msg, "%d.frm", frame);
		//U.quickSort(p2arena, 0, tmpTokenNum-1);
		//U.showArena(p2arena, hist, G, M, msg, tmpTokenNum);
		
	}
	#ifdef USE_LATTICE
	//U.showLattice(p2arena, hist, G, M, outputArena, tokenNum, frame, 0);
	#endif
}

void SEARCH::showResult(int topN, char *spk, char *feaFile){
	//char outputArena[MAXLEN];

	if(topN<0){
		printf("topN should be at least 1\n");
		exit(1);
	}
	if(topN>100)
		topN=100;

	U.quickSort(p2arena, 0, tokenNum-1);
	//U.showArena(p2arena, hist, G, M, "final.frm", 100);

	//strcpy(outputArena, "DecodeInfo_");
	//strcat(outputArena, strrchr(feaFile, '/')+1);
	//U.showArena(p2arena, hist, G, M, outputArena, 100);

	int node, branch, endNode, reachTerminal, token[100], topnNum=0;
	bool cond1, cond2;
	bool condx=false;

	for(int i=0;i<tokenNum;i++){
		node=p2arena[i]->node;
		branch=p2arena[i]->branch;
		endNode=G.arc[G.p2N[node]->arcPos+branch].endNode;

		(U.isTerminal(G.terminal, G.terminalNum, endNode))?cond1=true:cond1=false;
		if(G.arc[G.p2N[node]->arcPos+branch].in<=-DIC_OFFSET)
			cond2=false;
		else
			(D.p2MMF[G.arc[G.p2N[node]->arcPos+branch].in]->stateNum-3==p2arena[i]->state)?cond2=true:cond2=false;
		
		(cond1&&cond2)?reachTerminal=1:reachTerminal=0;

		if(reachTerminal){
			token[topnNum]=i;
			topnNum++;
			if(topnNum==topN)
				break;
		}		
	}
	// if no token reaches the terminal or Top-N list is not full, force output by score	
	if( (topnNum==0)||(topnNum<topN)){
		for(int i=0;i<tokenNum;i++){
			condx=false;
			for( int k=0; k<topnNum; k++ ){
                        	if( token[k]==i ){
					condx=true;
					break;
				}
			}
			if( condx ) continue;

			node=p2arena[i]->node;
			branch=p2arena[i]->branch;
			endNode=G.arc[G.p2N[node]->arcPos+branch].endNode;

			token[topnNum]=i;
			topnNum++;
			if(topnNum==topN)
				break;
		}
	}
        
	// standard output
	char decodedStr[MAXLEN*50], outWord[WORDLEN];
	if(topnNum==0)
		printf("NULL\t(%s_%s) 0\n", spk, feaFile);
	else{
		bool c1, c2, c3; // test SIL, EMPTY, </s>
		for(int t=0;t<topnNum;t++){
			int tokenIdx=token[t];
			strcpy(decodedStr, "");
			for(int j=0;j<hist[p2arena[tokenIdx]->hist].pathNum;j++){
				int pos=hist[p2arena[tokenIdx]->hist].arc[j];
				#ifdef USE_FRAMEWISE_OUTPUT
				strcpy(outWord, D.p2MMF[G.arc[pos].in]->str); // for framewise 
				#else
				strcpy(outWord, G.symbolList[G.arc[pos].out].str); 
				#endif
								
				(strcmp(outWord, SIL)==0)?c1=true:c1=false;
				(strcmp(outWord, EMPTY)==0)?c2=true:c2=false;
				(strcmp(outWord, "</s>")==0)?c3=true:c3=false;
			
				#ifdef USE_FRAMEWISE_OUTPUT
				if((c2==false)&&(c3==false)){
					strcat(decodedStr, D.p2MMF[G.arc[pos].in]->str); // for framewise
					strcat(decodedStr, " ");
				}
				#else
				// c1=false; c3=false; // show silence	
				if(((c1==false)&&(c2==false)&&(c3==false)))
				{// discard sil, <eps> and </s>
					strcat(decodedStr, G.symbolList[G.arc[pos].out].str);
					//sprintf(outWord, "%d", hist[p2arena[tokenIdx]->hist].frameBoundary[j]);
					//sprintf(outWord, "%d", j);
					//strcat(decodedStr, " ");
					//strcat(decodedStr, outWord);
					strcat(decodedStr, " ");
				}
				#endif
			}

			decodedStr[strlen(decodedStr)-1]='\0';
			
			#ifdef USE_LATTICE
			printf("%s\t(%s_%s) %f %f %f\n", decodedStr, spk, feaFile, p2arena[tokenIdx]->score, p2arena[tokenIdx]->amScore, p2arena[tokenIdx]->lmScore);
			#else
			printf("%s\t(%s_%s)\t%f", decodedStr, spk, feaFile, p2arena[tokenIdx]->score);
			#endif

			#ifdef USE_LATTICE
			/*
			if( t==0 ){
				char outputArena[100];
				sprintf( outputArena, "LatticeInfo_%d.txt", feaFileIndex);
				FILE *fp = fopen(outputArena, "at+");
				//fprintf(fp, "-1 %s\n", decodedStr);
				fprintf(fp, "-1 %s\n%d\n", decodedStr, hist[p2arena[token[0]]->hist].frameBoundary[hist[p2arena[token[0]]->hist].pathNum-1]-1);
				fclose(fp);
			}
			*/
			#endif
		}
	}

	// === CM3 === BEGIN
	/*char decodedStr[MAXLEN], tmp[100], outWord[WORDLEN];
	typedef struct {
		int boundStart;
		int boundEnd;
		int word;
	} CMSTR;
	int cmNum, wordCount, fStart, fEnd;

	CMSTR *cmStr = new CMSTR[MAXLEN];
	for(int i=0;i<MAXLEN;i++)
		cmStr[i].boundStart=0;
	
	if(topnNum==0)
		printf("wordCount\t0\nNULL\n(%s_%s) 0 ", spk, feaFile);
	else{
		int h=p2arena[token[0]]->hist; // top 1
		strcpy(decodedStr, "");
		cmNum=0;
		for(int j=0;j<hist[h].pathNum;j++){
			int pos=hist[h].arc[j];
			strcpy(outWord, G.symbolList[G.arc[pos].out].str);
			if( (strcmp(outWord, EMPTY)!=0)&&(strcmp(outWord, SIL)!=0) ){
				cmStr[cmNum].boundStart=hist[h].frameBoundary[j];
				cmStr[cmNum].boundEnd=hist[h].frameBoundary[j+1]-1;
				cmStr[cmNum].word=G.arc[pos].out;
				cmNum++;
			}
		}

		strcpy(decodedStr, "");
		wordCount=0;
		fStart=0;
		for(int i=0;i<cmNum;i++){
			strcpy(outWord, G.symbolList[cmStr[i].word].str);
			if(fStart==0)
				fStart=cmStr[i].boundStart;
			if(strcmp(outWord, "#")!=0){
				fEnd=cmStr[i].boundEnd;
				sprintf(tmp, "%s\t%d\t%d\n", G.symbolList[cmStr[i].word].str, fStart, fEnd);
				strcat(decodedStr, tmp);
				wordCount++;

				fStart=0;
			}
		}
		printf("wordCount\t%d\n%s", wordCount, decodedStr);
	}
	delete [] cmStr;*/
	// === CM3 === END
}

void SEARCH::resetGraphInfo(int option){
	int loopNum, loopS, loopE, pos;
	#ifdef ASR_SSE2
	  __m128i p, *sc;
	  __m128 q;
	#elif defined(ASR_AVX)
	__m256i p, *sc;
	__m256 q;
	#elif defined(ASR_NEON)
	int32x4_t p, *sc;
	float32x4_t q;
	#endif

	if(option==0){
		for(int i=0;i<beamSize;i++){
			hist[i].frame=-1;
			hist[i].pathNum=0;
		}
	}
	else{
		for(int x=0;x<tmpTokenNum;x++){
			//int pos=G.p2N[p2arena[x]->node]->arcPos+p2arena[x]->branch;
			int pos=arc2stateCompetePointer[G.p2N[p2arena[x]->node]->arcPos+p2arena[x]->branch]+p2arena[x]->phonePos; // @20120723
			arc2stateCompete[pos]=-1;
		}
		#if defined(ASR_SSE2)
  		  sc = (__m128i*) stateCompete;//@float
  		  p=_mm_set1_epi32(-1);//@float
  		  loopNum=(stateCompeteCounter*MAX_STATE_NUM>>2);//@float
  		  loopS=(loopNum<<2);//@float
  		  loopE=stateCompeteCounter*MAX_STATE_NUM;//@float
  		  for(int i=0;i<loopNum;i++){//@float
  			_mm_storeu_si128(sc, p);//@float
  			sc++;//@float
  		  }//@float
  		  for(int i=loopS;i<loopE;i++)//@float
  			stateCompete[i]=-1;//@float
		#elif defined(ASR_AVX)
  		  sc = (__m256i*) stateCompete;//@float
  		  p=_mm256_set1_epi32(-1);//@float
  		  loopNum=(stateCompeteCounter*MAX_STATE_NUM>>3);//@float
  		  loopS=(loopNum<<3);//@float
  		  loopE=stateCompeteCounter*MAX_STATE_NUM;//@float
  		  for(int i=0;i<loopNum;i++){//@float
  			_mm256_storeu_si256(sc, p);//@float
  			sc++;//@float
  		  }//@float
  		  for(int i=loopS;i<loopE;i++)//@float
  			stateCompete[i]=-1;//@float
		#elif defined(ASR_NEON)
  		  sc = (int32x4_t*) stateCompete;
		  p = vld1q_dup_s32((int32_t*)-1);
		  loopNum=(stateCompeteCounter*MAX_STATE_NUM>>2);
                  loopS=(loopNum<<2);
                  loopE=stateCompeteCounter*MAX_STATE_NUM;

  		  for(int i=0;i<loopNum;i++){
			//vst1q_s32(p, sc);
			*sc = p;
			sc++;
  		  }
  		  for(int i=loopS;i<loopE;i++)//@float
  			stateCompete[i]=-1;//@float

		#elif defined(ASR_FIXED)
		memset(stateCompete, -1, sizeof(int)*stateCompeteCounter*MAX_STATE_NUM);//@fixed		
		#else
		memset(stateCompete, -1, sizeof(int)*stateCompeteCounter*MAX_STATE_NUM);//@generic
		#endif
						
	}

	#if defined(ASR_SSE2)
	// DNN doesn't need cacheScore information anymore
	#elif defined(ASR_AVX)
	// DNN doesn't need cacheScore information anymore
	#elif defined(ASR_FIXED)
	memset(cacheScore, 0, sizeof(SCORETYPE)*M.tsNum);//@fixed
	#else
	//memset(cacheScore, 0, sizeof(SCORETYPE)*M.tsNum);//@generic	
	//memset(cacheScore, 0, sizeof(SCORETYPE)*D.DNNAM[D.numDnnComponent-1].outDim);//@generic
	#endif
	
	stateCompeteCounter=0;
}
/*
void SEARCH::computemixLogSumLookupTable(){
	double MIN_LOG_EXP=-23.02585;
	double MIN_LOG_EXP_STEP=0.00001;
	double diff;
	#if defined(ASR_SSE2)
	double scale[2]={-1/MIN_LOG_EXP_STEP, 0};
	#elif defined(ASR_AVX)
	double scale[4]={-1/MIN_LOG_EXP_STEP, 0, 0, 0};
	#else
	double scale=-1/MIN_LOG_EXP_STEP;
	#endif

	mixLogSumLookupTableSize=(int)(-MIN_LOG_EXP/MIN_LOG_EXP_STEP);
	mixLogSumLookupTable = (double*) U.aligned_malloc(mixLogSumLookupTableSize*sizeof(double), ALIGNSIZE);	
	#if defined(ASR_SSE2)
	mixLogSumLookupScale=_mm_load_sd(scale);
	#elif defined(ASR_AVX)
	mixLogSumLookupScale=_mm256_load_pd(scale);
	#else
	mixLogSumLookupScale=scale;
	#endif
	
	#pragma omp parallel for
	for(int i=0;i<mixLogSumLookupTableSize;i++){
		diff=-i*MIN_LOG_EXP_STEP;
		mixLogSumLookupTable[i]=log(1.0+exp(diff));
	}
}
*/
void SEARCH::readLookupTable(char *fileName, float alpha){
	size_t st;

	FILE *out=U.openFile(fileName, "rb");
	int ary1Size, ary2Size, ary3Size, ary4Size;
	st=fread(&ary1Size, sizeof(int), 1, out);//number of endNodes (nodes in a graph)
	st=fread(&ary2Size, sizeof(int), 1, out);//redirectNum of each node, represented in an array
	st=fread(&ary3Size, sizeof(int), 1, out);//set of endNodes
	st=fread(&ary4Size, sizeof(int), 1, out);//set of probs

	//printf("Lookup Table: %d %d %d %d\n", ary1Size, ary2Size, ary3Size, ary4Size);
	// "LT" denotes "Lookup Table"
	LTendNodeNum=new unsigned short int[ary1Size];
	//LTendNodeArrayPosition=new unsigned short int[ary2Size];
	LTendNodeArrayPosition=new unsigned int[ary2Size]; // LVCSR
	LTendNodeArray=new int[ary3Size];
	LTProb=new SCORETYPE[ary4Size];
	st=fread(LTendNodeNum, sizeof(unsigned short int), ary1Size, out);
	//st=fread(LTendNodeArrayPosition, sizeof(unsigned short int), ary2Size, out);
	st=fread(LTendNodeArrayPosition, sizeof(unsigned int), ary2Size, out); // LVCSR
	st=fread(LTendNodeArray, sizeof(int), ary3Size, out);
	st=fread(LTProb, sizeof(float), ary4Size, out);
	for(int i=0;i<ary4Size;i++)
		LTProb[i]=(SCORETYPE)(LTProb[i]*alpha);
	fclose(out);
}

void SEARCH::readDic(char *dicFile){
	size_t st;

	FILE *in=U.openFile(dicFile, "rb");
	st=fread(&dicEntryNum, sizeof(int), 1, in);
	dic = new DIC[dicEntryNum];
	st=fread(dic, dicEntryNum, sizeof(DIC), in);
	fclose(in);
}

void SEARCH::allocatestateCompete(){
	int stateCompeteSize=(beamSize*25)*MAX_STATE_NUM; // (beamSize*2~2.5) so as not to exceed the limit
	arc2stateCompeteNum=0;

	arc2stateCompetePointer = new unsigned int[G.arcNum]; // @20120723
	for(int i=0;i<G.arcNum;i++){
		arc2stateCompetePointer[i] = arc2stateCompeteNum; // @20120723
		if(G.arc[i].in>=0){ // HMM
			arc2stateCompeteNum++;
		}else if(G.arc[i].in==-1){// <eps>
			arc2stateCompeteNum++;
		}else{
			if(G.arc[i].in<-1){// D=#
				arc2stateCompeteNum+=dic[-(G.arc[i].in+DIC_OFFSET)].phoneNum;
			}
		}
	}
	arc2stateCompete = new int[arc2stateCompeteNum];
	stateCompete = new int[stateCompeteSize];
	memset(arc2stateCompete, -1, sizeof(int)*arc2stateCompeteNum);
	memset(stateCompete, -1, sizeof(int)*stateCompeteSize);
}

int SEARCH::obtainModel(int node, int branch, int phonePos){
	int MMFIndex, inLabel=G.arc[G.p2N[node]->arcPos+branch].in;
	(inLabel>=0)?MMFIndex=inLabel:MMFIndex=dic[-(inLabel+DIC_OFFSET)].phone[phonePos];

	return MMFIndex;
}

#if defined(USE_LSTM)
SEARCH::SEARCH(int keepTokenNum, float alpha, GRAPH &graph, LSTM &macro, char *lookupTableFile, char *dicFile)
#else
SEARCH::SEARCH(int keepTokenNum, float alpha, GRAPH &graph, DNN &macro, char *lookupTableFile, char *dicFile)
#endif
{
	D=macro; // acoustic model
	G=graph; // language model, sort of

	//maxTokenNum = 0;
	beamSize=keepTokenNum;
	(beamSize*5<500)?arenaSize=500:arenaSize=beamSize*50;

	arena = new ARENA[arenaSize];
	p2arena = new ARENA * [arenaSize];
	for(int i=0;i<arenaSize;i++){
		p2arena[i]=arena+i;
		p2arena[i]->node=0;
		p2arena[i]->branch=0;
		#ifdef USE_LATTICE
		p2arena[i]->amScore = 0.0;
		p2arena[i]->lmScore = 0.0;
		#endif
	}

	hist = new HISTORY[beamSize];
	//cacheScore = new SCORETYPE[M.tsNum];

	readLookupTable(lookupTableFile, alpha);
	readDic(dicFile);

	//computemixLogSumLookupTable();
	allocatestateCompete();
	epsIndex=U.stringSearch(G.symbolList, 0, G.symbolNum-1, EMPTY);
	if(epsIndex==-1){
		printf("%s not found in symbol list\n", EMPTY);
		exit(1);
	}
	
}

SEARCH::~SEARCH(){	
	delete [] arena;
	delete [] p2arena;
	delete [] hist;
	//delete [] cacheScore;
	delete [] LTendNodeNum;
	delete [] LTendNodeArrayPosition;
	delete [] LTendNodeArray;
	delete [] LTProb;
	delete [] dic;
	delete [] stateCompete;
	delete [] arc2stateCompete;	
	delete [] arc2stateCompetePointer;
	//U.aligned_free(mixLogSumLookupTable); // no use in DNN

	// DNN
	D.tm=NULL;
	D.p2tm=NULL;
	D.MMF=NULL;
	D.p2MMF=NULL;
	D.frameScore=NULL;
	#if not defined(USE_LSTM)
	D.DNNAM=NULL;
	D.Prior=NULL;
	#endif

	// Graph
	G.NO=NULL;
	G.p2NO=NULL;
	G.N=NULL;
	G.p2N=NULL;
	G.arc=NULL;
	G.terminal=NULL;
	G.symbolList=NULL;
}
