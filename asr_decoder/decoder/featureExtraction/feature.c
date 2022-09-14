#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
//#include <direct.h>
//#include "type_define.h"
//#include "variable.h"
////#include "variable.c"
#include "feature.h"
//#include "tcos.h"
////#include "tcos.c"

#include <unistd.h> // for Linux

char IN_WAV_DIR[256];
char OUT_C12_DIR[256];
char OUT_EC_DIR[256];
float C0;


//#define   IN_WAV_DIR    "/home1/e0mars5/santu/Feature/MAT2000/TR/Waveform/Clean/"
//#define   OUT_C12_DIR   "/home1/e0mars5/santu/Feature/MAT2000/TR/Clean/MFCC_C12"
//#define   OUT_EC_DIR    "/home1/e0mars5/santu/Feature/MAT2000/TR/Clean/MFCC_ENG"

const float FEATURE::tcos[CEP_DIM+1][FILTERNO+1] = {
{(float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000,
 (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000,
 (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000,
 (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000,
 (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000, (float)0.000000},
{(float)0.000000, (float)0.997859, (float)0.980785, (float)0.946930, (float)0.896873,
 (float)0.831470, (float)0.751840, (float)0.659346, (float)0.555570, (float)0.442289,
 (float)0.321439, (float)0.195090, (float)0.065403, (float)-0.065403, (float)-0.195090,
 (float)-0.321440, (float)-0.442289, (float)-0.555570, (float)-0.659346, (float)-0.751840,
 (float)-0.831470, (float)-0.896873, (float)-0.946930, (float)-0.980785, (float)-0.997859},
{(float)0.000000, (float)0.991445, (float)0.923880, (float)0.793353, (float)0.608761,
 (float)0.382683, (float)0.130526, (float)-0.130526, (float)-0.382683, (float)-0.608761,
 (float)-0.793353, (float)-0.923880, (float)-0.991445, (float)-0.991445, (float)-0.923880,
 (float)-0.793353, (float)-0.608761, (float)-0.382683, (float)-0.130526, (float)0.130526,
 (float)0.382684, (float)0.608762, (float)0.793353, (float)0.923880, (float)0.991445},
{(float)0.000000, (float)0.980785, (float)0.831470, (float)0.555570, (float)0.195090,
 (float)-0.195090, (float)-0.555570, (float)-0.831470, (float)-0.980785, (float)-0.980785,
 (float)-0.831470, (float)-0.555570, (float)-0.195090, (float)0.195090, (float)0.555570,
 (float)0.831470, (float)0.980785, (float)0.980785, (float)0.831470, (float)0.555570,
 (float)0.195090, (float)-0.195091, (float)-0.555570, (float)-0.831470, (float)-0.980785},
{(float)0.000000, (float)0.965926, (float)0.707107, (float)0.258819, (float)-0.258819,
 (float)-0.707107, (float)-0.965926, (float)-0.965926, (float)-0.707107, (float)-0.258819,
 (float)0.258819, (float)0.707107, (float)0.965926, (float)0.965926, (float)0.707107,
 (float)0.258819, (float)-0.258819, (float)-0.707107, (float)-0.965926, (float)-0.965926,
 (float)-0.707107, (float)-0.258819, (float)0.258819, (float)0.707107, (float)0.965926},
{(float)0.000000, (float)0.946930, (float)0.555570, (float)-0.065403, (float)-0.659346,
 (float)-0.980785, (float)-0.896873, (float)-0.442289, (float)0.195090, (float)0.751840,
 (float)0.997859, (float)0.831470, (float)0.321440, (float)-0.321439, (float)-0.831470,
 (float)-0.997859, (float)-0.751840, (float)-0.195091, (float)0.442289, (float)0.896873,
 (float)0.980785, (float)0.659346, (float)0.065403, (float)-0.555570, (float)-0.946930},
{(float)0.000000, (float)0.923880, (float)0.382683, (float)-0.382683, (float)-0.923880,
 (float)-0.923880, (float)-0.382683, (float)0.382684, (float)0.923880, (float)0.923879,
 (float)0.382683, (float)-0.382684, (float)-0.923880, (float)-0.923879, (float)-0.382683,
 (float)0.382684, (float)0.923880, (float)0.923879, (float)0.382683, (float)-0.382684,
 (float)-0.923880, (float)-0.923879, (float)-0.382683, (float)0.382684, (float)0.923880},
{(float)0.000000, (float)0.896873, (float)0.195090, (float)-0.659346, (float)-0.997859,
 (float)-0.555570, (float)0.321440, (float)0.946930, (float)0.831469, (float)0.065403,
 (float)-0.751840, (float)-0.980785, (float)-0.442288, (float)0.442289, (float)0.980785,
 (float)0.751839, (float)-0.065404, (float)-0.831470, (float)-0.946930, (float)-0.321438,
 (float)0.555571, (float)0.997859, (float)0.659345, (float)-0.195092, (float)-0.896873},
{(float)0.000000, (float)0.866025, (float)-0.000000, (float)-0.866025, (float)-0.866025,
 (float)0.000000, (float)0.866026, (float)0.866025, (float)-0.000000, (float)-0.866026,
 (float)-0.866025, (float)0.000000, (float)0.866026, (float)0.866025, (float)-0.000000,
 (float)-0.866026, (float)-0.866025, (float)0.000000, (float)0.866026, (float)0.866025,
 (float)-0.000001, (float)-0.866026, (float)-0.866025, (float)0.000001, (float)0.866026},
{(float)0.000000, (float)0.831470, (float)-0.195090, (float)-0.980785, (float)-0.555570,
 (float)0.555570, (float)0.980785, (float)0.195090, (float)-0.831470, (float)-0.831470,
 (float)0.195090, (float)0.980785, (float)0.555570, (float)-0.555570, (float)-0.980785,
 (float)-0.195090, (float)0.831470, (float)0.831470, (float)-0.195090, (float)-0.980785,
 (float)-0.555570, (float)0.555570, (float)0.980785, (float)0.195090, (float)-0.831470},
{(float)0.000000, (float)0.793353, (float)-0.382683, (float)-0.991445, (float)-0.130526,
 (float)0.923880, (float)0.608762, (float)-0.608761, (float)-0.923880, (float)0.130526,
 (float)0.991445, (float)0.382684, (float)-0.793353, (float)-0.793353, (float)0.382683,
 (float)0.991445, (float)0.130527, (float)-0.923879, (float)-0.608762, (float)0.608761,
 (float)0.923880, (float)-0.130526, (float)-0.991445, (float)-0.382684, (float)0.793353},
{(float)0.000000, (float)0.751840, (float)-0.555570, (float)-0.896873, (float)0.321440,
 (float)0.980785, (float)-0.065404, (float)-0.997859, (float)-0.195090, (float)0.946930,
 (float)0.442288, (float)-0.831470, (float)-0.659345, (float)0.659346, (float)0.831469,
 (float)-0.442290, (float)-0.946930, (float)0.195091, (float)0.997859, (float)0.065402,
 (float)-0.980786, (float)-0.321438, (float)0.896873, (float)0.555569, (float)-0.751841},
{(float)0.000000, (float)0.707107, (float)-0.707107, (float)-0.707107, (float)0.707107,
 (float)0.707107, (float)-0.707107, (float)-0.707107, (float)0.707107, (float)0.707107,
 (float)-0.707107, (float)-0.707106, (float)0.707107, (float)0.707106, (float)-0.707107,
 (float)-0.707106, (float)0.707107, (float)0.707106, (float)-0.707107, (float)-0.707106,
 (float)0.707107, (float)0.707106, (float)-0.707107, (float)-0.707106, (float)0.707107}
};


/***********************************
*        make directory
***********************************/
int FEATURE::MakeDirectory(char *filename)
   {
      int len, i;
      char pathfilename[512],newDir[512],currentPath[512],*ptr;
      char buffer[1024];

      len=0; ptr=pathfilename;
      while (filename[len]!='\0')
      {
         if (filename[len]=='/' || filename[len]=='\\')
         {
            //filename[len]='\\'; //run in window os, add #include <direct.h>
            *ptr=0;
            //  _mkdir(pathfilename);
            getcwd(currentPath,512);
            for(i=0; i<len; i++)
               newDir[i] = filename[i];
            newDir[i] = '\0';
            if(chdir(newDir)!=0)
            {
               chdir(currentPath);
               if(strlen(pathfilename)>0)
               {
                  sprintf(buffer,"mkdir %s",pathfilename);
                  system(buffer);
                  // or do //mkdir(pathfilename);
               }
            }
            chdir(currentPath);
         }
         *ptr++=filename[len++];
      }
      return(1);
   }
int FEATURE::filesize(FILE *stream)
{
   long curpos, length;

   curpos = ftell(stream);
   fseek(stream, 0L, SEEK_END);
   length = ftell(stream);
   fseek(stream, curpos, SEEK_SET);

   return (int)length;
}
void FEATURE::CEPLIFTER(int Frames)
{
	int i,j;
	float tmpf;
	float a,Lby2;    
	    
	    a = PI/22;
   Lby2 = 22/2.0;
   for(i=0;i<Frames;i++)
   {
      	for(j=0;j<CEP_DIM;++j)
      	{
      		//printf("t=%d j=%d %f ",i,j,gb_cep_data[i][j]);
      		tmpf=1.0 + Lby2*sin((j+1) * a);
        gb_cep_data[i][j]*=tmpf;
        //printf("%f\n",gb_cep_data[i][j]);
        //getchar();
      } 
    }
}
float FEATURE::Mel(int k)
{
	float	res;
	
	res = (float)1.0E7/(float)(SAMPPERIOD*FFT_ORDER*700.0);
	return ((float)(log(1+(k-1)*res)*1127.0));
}

void FEATURE::SetFilterBanks(double w[])
{
	int	b,Banks,n,curnB;
	float scale,melfreq;
	float centerF[FFT_ORDER_2];
	scale = Mel(FFT_ORDER_2+1);
	Banks = FILTERNO+1;
	for (b=0; b < Banks; b++)
	{
		centerF[b] = ((float)(b+1)/(float)Banks)*scale;
	}
	
	curnB = 0;
	for (n=0; n < FFT_ORDER_2; n++)
	{
		melfreq = Mel(n+1);
		gb_bank[n] = curnB;
		if (melfreq > centerF[curnB])
		{
			curnB++;
			gb_bank[n] = curnB;
		}
		if (curnB > 0)
		{
			w[n] = (centerF[curnB]-melfreq)/(centerF[curnB]-centerF[curnB-1]);
		}
		else
		{
			w[n] = (centerF[0]-melfreq)/centerF[0];
		}
	}
	return;
}

void FEATURE::PreEmphasis(float s[])
{
  int i;

  #ifdef SSE2
  
  __m128 *f1 = (__m128*)(s+FRAME_SIZE-4);
  //__m128 *f2 = (__m128*)(s+FRAME_SIZE-5); // address of f2 is not 16 bytes aligned!!! Crash!!!
  __m128 f2;
  __m128 d1;
  __m128 s1 = _mm_set1_ps(ALPHA);
  float *ptr = s+FRAME_SIZE-5;

  int loopNum = FRAME_SIZE>>2;
  for(i=0; i<loopNum-1; i++){

    f2 = _mm_set_ps(*(ptr+3), *(ptr+2), *(ptr+1), *ptr);
    d1 = _mm_mul_ps(f2, s1);
    *f1 = _mm_sub_ps(*f1, d1);

    f1--;
    ptr-=4;
  }
  d1 = _mm_set_ps(*(s+2), *(s+1), *s, *s);
  d1 = _mm_mul_ps(d1, s1);
  *f1 = _mm_sub_ps(*f1, d1);
  
  #else
  for (i=FRAME_SIZE-1;i>0;i--)
  {
     s[i] -= (s[i-1]*(float)ALPHA);
  }
  s[0] *= ((float)1.0-(float)ALPHA);
  #endif

  return;
}

void FEATURE::GetHammingWindow()
{
     int    n;
     double temp;
     temp = (2.0 * PI) / (FRAME_SIZE - 1);
     for (n=0; n<FRAME_SIZE; n++)
     {
         HW[n] = (float)(0.54 - 0.46*cos((double)n*temp));
         
     }
}

void FEATURE::Haming_window(float s[])
{
  int i;
  #ifdef SSE2
  int loopNum = FRAME_SIZE>>2;
  __m128 *f1 = (__m128*)s;
  __m128 *f2 = (__m128*)HW;

  for(i=0; i<loopNum; i++){
    *f1 = _mm_mul_ps( *f1, *f2 );
    f1++; f2++;
  }
  #else
  float temp;
  for (i=0;i < FRAME_SIZE;i++)
  {
      temp = (float)(s[i]*HW[i]);
	  s[i] = temp ;
  }
  #endif

  /*
  #ifdef _DEBUG
  for(i=0; i<FRAME_SIZE; i++)
    printf("%f ", s[i]);
  printf("\n");
  #endif
  */
  return;
}

void FEATURE::fft(int inv,int n,COMPLEX *z_data)
{
   int	m,
			le,
			ip,
			i,
			l,
			nm1,
			j,
			le1,
			nv2,
			k;
   COMPLEX u,ti,w;
   double	tmpx,tmpy;
   int fft_order;
   fft_order=FFT_ORDER;

   m=0;
   while(fft_order>1){
	   fft_order>>=1;
	   m++;
   }
	
   nv2=(int)((double)n/2.0);
   nm1=n-1;
   j=1;
   for(i=1;i<=nm1;++i)
   {
      if (i<j)
      {
		 tmpx=z_data[j].x;
         tmpy=z_data[j].y;
         z_data[j].x=z_data[i].x;
         z_data[j].y=z_data[i].y;
		 z_data[i].x=tmpx;
         z_data[i].y=tmpy;
      }
      k=nv2;
      while (k<j)
      {
         j=j-k;
         k=(int)((double)k/2.0);
      }
      j=j+k;
   }
   for(l=1;l<=m;++l)
   {
      le=(int)pow(2.0,(double)l);
      le1=(int)((double)le/2.0);
      u.x=1.0;
      u.y=0.0;
      w.x= cos(PI/(double)le1);
      w.y= sin(PI/(double)le1)*(-inv);
      for(j=1;j<=le1;++j)
      {
         i=j;
         while (i<=n)
         {
			 ip=i+le1;
			 ti.x=z_data[ip].x*u.x-z_data[ip].y*u.y;
			 ti.y=z_data[ip].x*u.y+z_data[ip].y*u.x;
			 z_data[ip].x=z_data[i].x-ti.x;
			 z_data[ip].y=z_data[i].y-ti.y;
			 z_data[i].x=z_data[i].x+ti.x;
			 z_data[i].y=z_data[i].y+ti.y;
			 i=i+le;
         }
		 tmpx=u.x;
		 u.x=tmpx*w.x-u.y*w.y;
		 u.y=tmpx*w.y+u.y*w.x;
      }
   }
   if (inv== -1)
   {
      for (i=1;i<=n;++i)
      {
			z_data[i].x=z_data[i].x/(double)n;
			z_data[i].y=z_data[i].y/(double)n;
      }
   }
   return;
}
void FEATURE::SubDC(float s[])
{
	double sum,y,mean;
	int i;
	sum=0;
	
	#ifdef SSE2
	ALIGNTYPE float ss[4];

	int loopNum;
	__m128 *f1 = (__m128*)s;
	__m128 *f2 = (__m128*)(s+(FRAME_SIZE>>1));
	__m128 s1, s2;

	s1=_mm_setzero_ps();
	s2=_mm_setzero_ps();

	loopNum=FRAME_SIZE>>3;
	for(i=0; i<loopNum; i++){
		s1 = _mm_add_ps( *(f1++), *(f2++) );
		s2 = _mm_add_ps( s2, s1 );
	}
	_mm_store_ps(ss, s2);
	ss[0]=ss[0]+ss[1]+ss[2]+ss[3];
	ss[1]=ss[0]/(float)FRAME_SIZE;

	f1 = (__m128*)s;
	s1 = _mm_set1_ps(ss[1]);
	loopNum=FRAME_SIZE>>2;	
	for(i=0; i<loopNum; i++){
		*f1 = _mm_sub_ps(*f1, s1);
		f1++;
	}

	#elif defined(AVX)
	ALIGNTYPE float ss[8];
	int loopNum;
	__m256 *f1 = (__m256*)s;
	__m256 *f2 = (__m256*)(s+(FRAME_SIZE>>1));
	__m256 s1, s2;

	s1=_mm256_setzero_ps();
	s2=_mm256_setzero_ps();

	loopNum=FRAME_SIZE>>4;
	for(i=0; i<loopNum; i++){
		s1 = _mm256_add_ps( *(f1++), *(f2++) );
		s2 = _mm256_add_ps( s2, s1 );
	}
	_mm256_store_ps(ss, s2);
	ss[0]=ss[0]+ss[1]+ss[2]+ss[3]+ss[4]+ss[5]+ss[6]+ss[7];
	ss[1]=ss[0]/(float)FRAME_SIZE;

	f1 = (__m256*)s;
	s1 = _mm256_set1_ps(ss[1]);
	loopNum=FRAME_SIZE>>3;
	for(i=0; i<loopNum; i++){
		*f1 = _mm256_sub_ps(*f1, s1);
		f1++;
	}	
	
	#else
	for(i=0;i<FRAME_SIZE;i++)
	{
		sum+=s[i];
	}
	mean=sum/(float)FRAME_SIZE;

	for(i=0;i<FRAME_SIZE;i++)
	{
		y=(double)s[i]-mean;
		s[i]=(float)y;
	}
	#endif

	#ifdef _DEBUG
	/*
	for(i=0;i<FRAME_SIZE;i++)
		printf("%f ",s[i]);
	printf("\n");
	*/
	#endif


}
int FEATURE::MelBandMagnitude(short *sample)
{
   int k,i,j,t,b,wait,errorCode;
	float abs_z, pb, pb1;

	double power_z;
	double sum;
	
    	COMPLEX z[FFT_ORDER+1];
	int totalFrames;
	int frameSize,frameRate;

	#if defined(SSE)||defined(AVX)
	ALIGNTYPE float s[FRAME_SIZE];
	#else
	float s[FRAME_SIZE];
	#endif

	errorCode=1;
    frameRate=FRAME_SHIFT;
    frameSize=FRAME_SIZE;
	//gb_totalFrames = (sampleNumber-(FRAME_SIZE-FRAME_SHIFT))/FRAME_SHIFT;
    totalFrames=gb_totalFrames;
	wait=-1;


   for (t=0; t<totalFrames; t++)
   {
	sum = 0.0;
	for (j=0; j<frameSize; j++) 
        {
         s[j] = (float)sample[t*frameRate+j];
         sum += s[j]*s[j];
        }

	if(sum == 0)
	{
	  if(wait == -1)
	  {
		  wait=t;
	  }
	}
	else
	{
		  gb_eng[t] = (float)10.0*(float)log10(sum/(float)frameSize);
		  if(wait != -1)
		  {
			  for(j=wait;j < t;j++)
			  {
				  gb_eng[j]=gb_eng[t];
			  }
			  wait=-1;
		  }
	  }

	#ifdef _DEBUG
	clock_t t1, t2;
	t1=clock();	
	//for(int zz=0; zz<1000000; zz++)
	#endif
	
	SubDC(s); // SSE2 => 4x faster; AVX => 7x faster

	#ifdef _DEBUG
	t2=clock();
	printf("%ld %d %d %g\n", sizeof(clock_t), t1, t2, (double)(t2-t1)/CLOCKS_PER_SEC);
	#endif

	PreEmphasis(s); // SSE2 => 2x faster
	Haming_window(s); // SSE2 => 1.5x faster	
	

      for (i=0;i <FFT_ORDER;i++)
      {
         z[i+1].x = 0.0;
         z[i+1].y = 0.0;
      }
      for (i=0;i < frameSize;i++)
         z[i+1].x = s[i];

      fft(1,FFT_ORDER,z);

      for (b=1; b <= FILTERNO; b++)
	      gb_band_eng[t][b] = (float)0.0;
		
	  
      for (k=2; k <= FFT_ORDER_2; k++)
      {
		 power_z = z[k].x*z[k].x+z[k].y*z[k].y;
         abs_z = (float)sqrt(power_z);
         b = gb_bank[k-1];
         pb = (float)(gb_fft_wt[k-1]*abs_z);
		pb1 = (float)(gb_fft_wt[k-1]*power_z);
		if (b > 0)
		{
			gb_band_eng[t][b]+=pb;
		}
         if (b < FILTERNO)
		 {
            gb_band_eng[t][b+1] += abs_z-pb;

		 }
      }
	  
      
   }//for (t=0; t<totalFrames; t++)


   return(errorCode);
}


void FEATURE::DCT(CEP_DATA cep[])
{
	int i,j,t;
	double pp;
	double temp;
	int totalFrames;
    float power[FILTERNO+1];
	float mfnorm;
	totalFrames=gb_totalFrames;
	mfnorm = (float)sqrt(2.0/((float)FILTERNO));
	for(t=0;t<totalFrames;t++)
    {
		temp=0;
		for(i=0;i<=FILTERNO;i++)
			power[i]=(float)gb_band_eng[t][i];
		
		if(EC){
			EnergyContour(&gb_eng[t],gb_band_eng[t]);
		}
		for(i=1;i<=FILTERNO;i++)
        {
			if(gb_band_eng[t][i]<1.0) gb_band_eng[t][i]=1.0;
			gb_band_eng[t][i] = log(gb_band_eng[t][i]);
		} 
		for (i=0; i < CEP_DIM; i++)
		{
			pp = (float)0.0;
			for (j=1; j <= FILTERNO; j++)
			{
				pp += gb_band_eng[t][j]*tcos[i+1][j];
				//tcos[][] are pre-computed values of cos(PI*(i+1)/24*(j-0.5))
			}
			pp *= mfnorm;
			cep[t][i]=(float)pp;
		}
		C0 = 0.0;
		for (j=1; j <= FILTERNO; j++)
		{
			C0 += gb_band_eng[t][j];
			//tcos[][] are pre-computed values of cos(PI*(i+1)/24*(j-0.5))
		}
		C0*=mfnorm;
		cep[t][CEP_DIM]=C0;
		//printf("%f %f\n",C0,cep[t][CEP_DIM]);
		
		//fpmc
		/*
		C0[t]=0;
		for(j=1;j<=FILTERNO;j++) C0[t]+=DCT[0][j-1]*gb_band_eng[t][j];
		*/
		
	} // loop t
}

void FEATURE::EnergyContour(float* eng,double *pwr)
{
   int b;
   double temp;
   temp=0;
   for(b=6;b<=FILTERNO;b++){
	   temp+=pwr[b];
   }
   *eng=(float)10.0*(float)log10(temp);
   
}

//Romove long term average of power spectrum
void FEATURE::LTA222()
{
  int i,j,k,m,dis_no;
  double mf[FILTERNO+1],tf,thld[FILTERNO+1];
  int frm_no;
  frm_no=gb_totalFrames;
  
  for(i=0;i<=FILTERNO;i++) mf[i]=0.0;
  
  m=0;
  if(CUTPURENOISE) dis_no=NOISE_FRM;
  else dis_no=0;
  
  for(i=dis_no;i<frm_no-dis_no;i++)
  {
      m++;
      for(j=1;j<=FILTERNO;j++) mf[j]+=gb_band_eng[i][j]*gb_band_eng[i][j];
  }
  for(i=1;i<=FILTERNO;i++)
  {
      mf[i]/=(double) m;
      thld[i]=ALPHA_LTA/(1.0-BETA)*mf[i];
  } 
  k=m=0;   
  for(i=0;i<frm_no;i++)
  {
	  
	  for(j=1;j<=FILTERNO;j++) 
	  {
		  tf=gb_band_eng[i][j]*gb_band_eng[i][j];
		  if(tf>thld[j])
		  { 
			  tf=tf-ALPHA_LTA*mf[j]; 
			  if(i>=dis_no&&i<frm_no-dis_no) 
				  k++; 
			  else 
				  m++; 
		  }
		  else 
			  tf*=BETA;
		  
		  gb_band_eng[i][j]=sqrt(tf);
	  }
  }  
} // end of LTA()

int FEATURE::getTotalFrame(int sampleNumber)
{
	return((sampleNumber-(FRAME_SIZE-FRAME_SHIFT))/FRAME_SHIFT);
}

void FEATURE::ini_fea_para()
{
  SetFilterBanks(gb_fft_wt);
  GetHammingWindow();
}

void FEATURE::getDeltaCep(int totalFrames,CEP_DATA *cep,CEP_DATA *deltaCep)
{
	int	k,i,t,lastT;
   float sigmaT2,sum;

	//Compute delta cepstrum
   sigmaT2 = (float)0.0;
   for (t=1; t<=DELTA; t++)
	  sigmaT2 += t*t;
	
   sigmaT2 *= (float)2.0;

   //frome frame 0 to frame DELTA
   for (t=0; t < DELTA; t++) 
	{
      for (i=0; i < CEP_DIM; i++)
		{
         deltaCep[t][i]=cep[t+1][i] - cep[t][i];
		}
   }

   //between DELTA to Tot - DELTA frames
   lastT = totalFrames - DELTA;
   for (t=DELTA; t < lastT; t++)
   {
      for (i=0; i < CEP_DIM; i++)
      {
         sum = (float)0.0;
         for (k=1; k <= DELTA; k++)
		 {
            sum += k*(cep[t+k][i]-cep[t-k][i]);
		 }
         deltaCep[t][i]=sum/sigmaT2;
      }
   }

   //from frame Tot - DELTA to frame Tot
   for (t=lastT; t < totalFrames; t++) 
   {
      for (i=0; i < CEP_DIM; i++)
	  {
         deltaCep[t][i]=cep[t][i]-cep[t-1][i];
	  }
   }
	for (t=0; t < totalFrames; t++) 
   {
      for (i=0; i < CEP_DIM; i++)
	  {
        printf("t=%d D=%d f=%f\n",t,i, deltaCep[t][i]);
        getchar();
	  }
   }
	return;
}
void FEATURE::getDeltaCep_New(int totalFrames,CEP_DATA *cep,CEP_DATA *deltaCep)
{
   int k,i,t;
   float sigmaT2,sum;
   //int index, lastT;

	//Compute delta cepstrum
   sigmaT2 = (float)0.0;
   for (t=1; t<=DELTA; t++)
	  sigmaT2 += t*t;
	
   sigmaT2 *= (float)2.0;

   for (t=0; t < totalFrames; t++)
   {
	   for (i=0; i < CEP_DIM+1; i++)
	   {
		   sum = (float)0.0;
		   for (k=1; k <= DELTA; k++)
		   {
			   if(t-k <0)
			   {
				   sum += k*(cep[t+k][i]-cep[0][i]);
			   }
			   else if(t+k >=totalFrames)
			   {
				   sum += k*(cep[totalFrames-1][i]-cep[t-k][i]);
			   }
			   else
			   {
				   sum += k*(cep[t+k][i]-cep[t-k][i]);
			   }
		   }
		   deltaCep[t][i]=sum/sigmaT2;
	   }
   }
   /*
for (t=0; t < totalFrames; t++) 
   {
      for (i=0; i < CEP_DIM+1; i++)
	  {
        printf("t=%d D=%d f=%f %f\n",t,i, cep[t][i],deltaCep[t][i]);
        getchar();
	  }
   }*/
	return;
}
void FEATURE::getDDEng(int totalFrames,float *eng,float *dEng,float *ddEng)
{
   int	k,t,lastT;
   float sigmaT2,sum;

	//Compute delta cepstrum
   sigmaT2 = (float)0.0;
   for (t=1; t<=DELTA; t++)
	   sigmaT2 += t*t;
	sigmaT2 *= (float)2.0;

   //frome frame 0 to frame DELTA
   for (t=0; t < DELTA; t++) 
     dEng[t]=eng[t+1] - eng[t];
  

   //between DELTA to Tot - DELTA frames
   lastT = totalFrames - DELTA;
   for (t=DELTA; t < lastT; t++)
   {
      sum = (float)0.0;
      for (k=1; k <= DELTA; k++)
	  {
         sum += k*(eng[t+k]-eng[t-k]);
	  }
      dEng[t]=sum/sigmaT2;
   }

   //from frame Tot - DELTA to frame Tot
   for (t=lastT; t < totalFrames; t++) 
   {
      dEng[t]=eng[t]-eng[t-1];
   }
	
	//delta delta energy
   for (t=0; t < DDELTA; t++)
   {
      ddEng[t]=dEng[t+1] - dEng[t];
   }

   lastT = totalFrames - DDELTA;
   for (t=DDELTA; t < lastT; t++)
   {
      ddEng[t]=dEng[t+1]-dEng[t-1];
   }

   for (t=lastT; t < totalFrames; t++)
   {
      ddEng[t]=dEng[t]-dEng[t-1];
   }

	return;
}


//int MFCC_shell(char *fname,FEA_DATA fea[])
int FEATURE::MFCC_shell(char *fname)
{ 
   FILE *fp;
   short *wave, *tmpWave;
   int samples_no;
   int t,d;
   int begint,endt;
   int frm_no;

   double  sample_mean;
   short   headerLength;

   int     max_samples_no, max_gb_totalFrames, K_pt, shiftN;
   int     *tmpFrame;
   float   *tmpEng;

   CEP_DATA /* *cep_data, */ *dcep_data, *tmpCep_data;
 
   fp=fopen(fname,"rb");
   if(!fp) {
	   printf("can't open file %s\n",fname);
	   exit(1);
   }

   //samples_no=filesize(fp);
   wave=(short*)calloc(sizeof(short),MAX_SAMPLE_NO);

   if(WAV_VAT==1)
   {//VAT file header
      fread(&headerLength,sizeof(short),1,fp);
      if (headerLength==1)	
      {
         fseek(fp,256,SEEK_SET);
      }
      else if (headerLength==2) 
      {
         fseek(fp,512,SEEK_SET); 
      }
   }
   else
   {//WAV file
        char header[5];
	fread(&header, sizeof(char), 4, fp);
	header[4] = '\0';
	if( strcmp( header, "RIFF" )==0 ){
		// header WAV PCM file
		fseek(fp, 44, SEEK_SET);
	}else{
		// headerless PCM file
   		fseek(fp, FILE_HEADER_LEN, SEEK_SET);
	}
   }

   samples_no=fread(wave,sizeof(short),MAX_SAMPLE_NO,fp);
   gb_totalFrames=getTotalFrame(samples_no);
   //cep_data=calloc(sizeof(CEP_DATA),gb_totalFrames);
   gb_cep_data=(CEP_DATA*)calloc(sizeof(CEP_DATA),gb_totalFrames);
   //dcep_data=calloc(sizeof(CEP_DATA),gb_totalFrames);
   
/*
   //DC substration
   sample_mean = 0;
   for(t=0; t<samples_no; t++)
   {
      sample_mean += (double)wave[t];
   }
   sample_mean /= (double)samples_no;
   for(t=0; t<samples_no; t++)
   {
      wave[t] -= (short)sample_mean;
   }
*/

   //initialize
	max_samples_no = samples_no;
	max_gb_totalFrames = gb_totalFrames;

	tmpCep_data=(CEP_DATA*)calloc(sizeof(CEP_DATA),max_gb_totalFrames);
	if(tmpCep_data==NULL)
	{
		printf("memory allocate error\n");		
	}
	tmpFrame = (int*)malloc(sizeof(int)*K_POINT);
	if(tmpFrame==NULL) 
	{
		printf("memory allocate error\n");		
	}
	tmpEng   = (float*)malloc(sizeof(float)*max_gb_totalFrames);
	if(tmpEng==NULL) 
	{
		printf("memory allocate error\n");
	}

	for (t=0; t<max_gb_totalFrames; t++)
	{
		tmpEng[t] = 0.0f;
		for (d=0; d<CEP_DIM; d++)
			tmpCep_data[t][d] = 0.0f;
	}
	
	for(K_pt=K_POINT-1; K_pt>=0; K_pt--)
	{
		shiftN = (int)K_pt*((int)FRAME_SHIFT/(float)K_POINT);
		
		tmpWave = wave+shiftN;
		samples_no = max_samples_no - shiftN;
		gb_totalFrames=getTotalFrame(samples_no);
		tmpFrame[K_pt] = gb_totalFrames;
		
		//neet gb_totalFrames, FRAME_SHIFT, FRAME_SIZE,  get gb_band_eng[t][b+1]
		MelBandMagnitude(tmpWave);
		if(LTA)
		{//need gb_band_eng[t][b+1], gb_totalFrames, modify gb_band_eng[t][b+1]
			LTA222(); 
		}	   
		DCT(gb_cep_data);
		
		
		//sum gb_cep_data
		for (t=0; t<gb_totalFrames; t++)
		{
			tmpEng[t] += gb_eng[t];
			for (d=0; d<CEP_DIM; d++)
				tmpCep_data[t][d] += gb_cep_data[t][d];
		}
	}

   //avg. tmpCep_data[t][d], tmpEng[t] -> gb_cep_data[t][d], gb_eng[t]
	samples_no = max_samples_no;
	gb_totalFrames = max_gb_totalFrames;
	for (t=0; t<gb_totalFrames; t++)
	{
		if(t != gb_totalFrames-1 || tmpFrame[K_POINT-1] == gb_totalFrames)
		{//do for last frame
			gb_eng[t] = tmpEng[t]/(float)K_POINT;
			for (d=0; d<CEP_DIM; d++)
				gb_cep_data[t][d] = tmpCep_data[t][d]/(float)K_POINT;
		}
		else //use the last time result: gb_eng[t], gb_cep_data[t][d]
		{ 
			//printf(" frame have less\n");
		}
	}

   
   if(CUTPURENOISE)
   {
	   begint=NOISE_FRM;
	   endt=gb_totalFrames-NOISE_FRM;
	   
   }
   else
   {
   	   begint=0;
	   endt=gb_totalFrames;
   }
   frm_no=endt-begint;


/*
   CMN(cep_data,begint,endt,frm_no);
   getDeltaCep(gb_totalFrames,cep_data,dcep_data);
   getDDEng(gb_totalFrames,gb_eng,gb_deng,gb_ddeng);
   for(t=0;t<gb_totalFrames;t++){
	  for(d=0;d<CEP_DIM;d++)
        fea[t][d]=cep_data[t][d];
	  
	  for(d=0;d<CEP_DIM;d++)
        fea[t][d+CEP_DIM]=dcep_data[t][d];
      
	  fea[t][DIM-2]=gb_deng[t];
	  fea[t][DIM-1]=gb_ddeng[t];

   }
*/
   //free(cep_data);
   //if(dcep_data)
   	//free(dcep_data);
   
   if(wave)
	   	free(wave);
   if(tmpCep_data)
		free(tmpCep_data);
   if(tmpEng)
	   free(tmpEng);
   if(tmpFrame)
	   free(tmpFrame);
   if(fp)
   		fclose(fp);

   return gb_totalFrames;
}

void FEATURE::CMN(CEP_DATA *Cep_Data,int begin,int end,int frameNumber)
{
	int	i,t;
	double cmean[CEP_DIM];
    
	//cepstrum mean normalize
	//printf("%d %d %d\n",begin,end,frameNumber);
	for (i=0; i<CEP_DIM ;i++)
	{
		cmean[i] = 0.0f;
		for (t=begin; t<end; t++)
		{
			cmean[i] += Cep_Data[t][i];
		}
		cmean[i] /= (float)frameNumber;
	}
   
   for (t=0; t<frameNumber+2*NOISE_FRM; t++)
	{
		for (i=0; i<CEP_DIM; i++)
		{
			//printf("%f %f\n",Cep_Data[t][i],cmean[i]);
			Cep_Data[t][i] -= cmean[i];    
			//printf("%f %f\n",Cep_Data[t][i],cmean[i]);
			getchar();
		}
	}

	return;
}
void FEATURE::MVN(OUT_DATA *Cep_Data,int frameNumber)
{
   int   t, d;
   double mean[OUT_DIM], sd[OUT_DIM];

   int   i;
   double *fea;

   for (d=0; d<OUT_DIM; ++d)
   {
      mean[d] = 0.0f;
      sd[d] = 0.0f;
   }

   for (t=0; t<frameNumber; ++t)
      for (d=0; d<OUT_DIM; ++d)
      {
         mean[d] += Cep_Data[t][d];
         sd[d]  += Cep_Data[t][d]*Cep_Data[t][d];
      }

   for (d=0; d<OUT_DIM; ++d)
   {
      mean[d] /= (float)frameNumber;
      sd[d] = (float)sqrt(sd[d]/(float)frameNumber - mean[d]*mean[d]);
   }

   for (t=0; t<frameNumber; ++t)
      for (d=0; d<OUT_DIM; ++d)
         Cep_Data[t][d] = (Cep_Data[t][d] - mean[d])/sd[d];
   
   if(DO_AVG==1)
   {
      fea = (double *) malloc(sizeof(double)*frameNumber);
      for(d=0; d<OUT_DIM; d++)
      {
         for (t=ORDER_M; t<(frameNumber-ORDER_M); ++t)
         {
           fea[t] = 0.0f;
           for(i=(-ORDER_M); i<=ORDER_M; i++)
              fea[t] += Cep_Data[t+i][d];

      	    fea[t] /= (double)(2.0f*ORDER_M+1.0f);
         }

         //assign Cep_Data[t][d]
         for (t=ORDER_M+1; t<(frameNumber-ORDER_M); ++t)
            Cep_Data[t][d] = (float)fea[t];
      }
      if(fea)
      {
         free(fea);
         fea = NULL;
      }
   }
   else if(DO_AVG==2)
   {
      //do ARMA
      fea = (double *) malloc(sizeof(double)*1);
      for(d=0; d<OUT_DIM; d++)
      {
         for (t=ORDER_M; t<(frameNumber-ORDER_M); ++t)
         {
            *fea = 0.0f;
	    for(i=1; i<=ORDER_M; i++)
	    {
               (*fea) += Cep_Data[t+i][d];
               (*fea) += Cep_Data[t-i][d];
            }
            Cep_Data[t][d] += (float)(*fea);
      	    Cep_Data[t][d] /= (float)(2.0f*ORDER_M+1.0f);
         }
      }
      if(fea)
      {
         free(fea);
         fea = NULL;
      }

   }
}
void FEATURE::CMN_New(CEP_DATA *Cep_Data,int frameNumber)
{
	int	i,t;
	double cmean[CEP_DIM+1];
    
	//cepstrum mean normalize
	//printf("%d\n",frameNumber);
	for (i=0; i<CEP_DIM+1 ;i++)
	{
		cmean[i] = 0.0f;
		for (t=0; t<frameNumber; t++)
		{
			cmean[i] += Cep_Data[t][i];
		}
		cmean[i] /= (float)frameNumber;
	}
   
   for (t=0; t<frameNumber; t++)
	{
		for (i=0; i<CEP_DIM+1; i++)
		{
			//printf("%f %f\n",Cep_Data[t][i],cmean[i]);
			Cep_Data[t][i] -= cmean[i];    
			//printf("%f %f\n",Cep_Data[t][i],cmean[i]);
			//getchar();
		}
	}

	return;
}
// output to file   //gb_cep_data[t][dim],gb_eng[t]   , gb_totalFrames
int FEATURE::OutputFeature(char *fileName)
{
	char   fname[512];
	int    i;
	FILE   *fp_C12, *fp_ENG;

	// C12 (EC_LTA_C12)
	sprintf(fname,"%s/%s.C12", OUT_C12_DIR, fileName);
	MakeDirectory(fname);
	fp_C12 = fopen(fname,"wb");
	if(fp_C12 == NULL)
	{
		printf("open C12 to write ERROR : %s\n", fname);
		return(0);
	}
	fwrite(&gb_totalFrames,sizeof(int),1,fp_C12);
	//write log10 eng
	for(i=0;i < gb_totalFrames;i++)
	{
		fwrite(gb_cep_data[i],sizeof(float)*CEP_DIM,1,fp_C12);
	}
	if(fp_C12)
		fclose(fp_C12);
	

	// ENG (EC_LTA_ENG)
	sprintf(fname,"%s/%s.ENG", OUT_EC_DIR, fileName);
	MakeDirectory(fname);
	fp_ENG = fopen(fname,"wb");
	if(fp_ENG == NULL)
	{
		printf("open ENG to write ERROR : %s\n", fname);
		return(0);
	}
	fwrite(&gb_totalFrames,sizeof(int),1,fp_ENG);
	//write log10 eng
	fwrite(gb_eng,sizeof(float),gb_totalFrames,fp_ENG);

	if(fp_ENG)
		fclose(fp_ENG);
	return(1);
}
void FEATURE::byteSwapShort(short *s)
 {
      char v;
      char *tmp = (char *)s ;
      v = *tmp;
      *tmp = *(tmp+1);
      *(tmp+1) = v;
   }
void FEATURE::byteSwapInt(int *s) 
	{
      char v;
      char *tmp = (char *)s ;
      v = *(tmp+1);
      *(tmp+1) = *(tmp+2); *(tmp+2) = v;
      v = *(tmp);
      *(tmp) = *(tmp+3);
      *(tmp+3) = v;
   }
void FEATURE::byteSwapArray(float *s, int d) 
	{
		int i;
      for(i=0;i<d;i++)
	  {
         char v;
         char *tmp = (char *)(s+i) ;
         v = *(tmp+1);
         *(tmp+1) = *(tmp+2); *(tmp+2) = v;
         v = *(tmp);
         *(tmp) = *(tmp+3);
         *(tmp+3) = v;
      }
   }
int FEATURE::OutputFeatureHTKFormat(char *fileName,char *extension)
{
	char   fname[512];
	int    i,t,d;
	FILE   *fp_fea;
	int nSamples, sampPeriod;
	short sampSize, parmKind;
    
	float data[OUT_DIM];
	// C12 (EC_LTA_C12)
	sprintf(fname,"%s/%s.%s", OUT_C12_DIR, fileName, extension);
	MakeDirectory(fname);
	fp_fea = fopen(fname,"wb");
	
	nSamples=(int)gb_totalFrames;
		sampPeriod=100000;
		sampSize=(int)sizeof(float)*OUT_DIM;
		parmKind=6;
		byteSwapShort(&sampSize); 
		byteSwapShort(&parmKind);
		byteSwapInt(&nSamples); 
		byteSwapInt(&sampPeriod);
		fwrite(&nSamples,   sizeof(int),  1, fp_fea);
		fwrite(&sampPeriod, sizeof(int),  1, fp_fea);
		fwrite(&sampSize,   sizeof(short), 1, fp_fea);
		fwrite(&parmKind,   sizeof(short), 1, fp_fea);
	for(t=0;t<gb_totalFrames;t++)
	{
		for(d=0;d<OUT_DIM;d++)
		{
			data[d]=out_data[t][d];
		}
		byteSwapArray(data,OUT_DIM);
		fwrite(data,sizeof(float),OUT_DIM,fp_fea);
	}
	if(fp_fea)
		fclose(fp_fea);
	
	return(1);
}
int FEATURE::AssignCepToOutData(void)
{
	int t,d;

	for(t=0;t<gb_totalFrames;t++)
	{
		for(d=0;d<CEP_DIM+1;d++)
		{
			out_data[t][d]=gb_cep_data[t][d];
			out_data[t][d+CEP_DIM+1]=delta_data[t][d];
			out_data[t][d+CEP_DIM*2+2]=delta2_data[t][d];
			//printf("%f %f %f\n",gb_cep_data[t][d],delta_data[t][d],delta2_data[t][d]);
			//getchar();
		}
	}/*
	for(t=0;t<gb_totalFrames;t++)
	{
		for(d=0;d<OUT_DIM;d++)
		{
			printf("t=%d D=%d %f\n",t,d,out_data[t][d]);
			getchar();
		}
	}
	*/
	return 1;
}

void FEATURE::doInitial(){
  ini_fea_para();
}

void FEATURE::doExtract(char *fname, char *fileName, int outMode){

	MFCC_shell(fname);
	CEPLIFTER(gb_totalFrames);
	out_data=(OUT_DATA*)calloc(sizeof(OUT_DATA),gb_totalFrames);
	delta_data=(CEP_DATA*)calloc(sizeof(CEP_DATA),gb_totalFrames);
	delta2_data=(CEP_DATA*)calloc(sizeof(CEP_DATA),gb_totalFrames);

	if(DO_CMN_OR_MVN==0)
		CMN_New(gb_cep_data,gb_totalFrames);

	getDeltaCep_New(gb_totalFrames,gb_cep_data,delta_data);
	getDeltaCep_New(gb_totalFrames,delta_data,delta2_data);
	AssignCepToOutData();

	if(DO_CMN_OR_MVN==1)
		 MVN(out_data,gb_totalFrames);

	if( outMode==1 ){
		if(!OutputFeatureHTKFormat(fileName,extensionName))
		{
			printf("Error\n");
			getchar();			
		}
	}else if(outMode==2){
		printf("%s\n", fileName);
		#ifdef _DEBUG
		#else
		for(int i=0; i<gb_totalFrames; i++){
			printf("\t%d:", i);
			for(int j=0; j<OUT_DIM; j++){
				printf(" %f", out_data[i][j]);
			}
			printf("\n");
		}
		#endif
	}
		
	if(gb_cep_data)
		free(gb_cep_data);
	if(delta_data)
		free(delta_data);
	if(delta2_data)
		free(delta2_data);

}

void FEATURE::doReset(){
  if(out_data)
    free(out_data);
}


void FEATURE::checkArgs(int argc, char *argv[]){
  if(argc!=5)
  {
    printf("feature.exe <list file> <IN_WAV_DIR> <OUT_DIR> <CMN/MVNMA/NONE>\n");
    exit(0);
  }

  DO_CMN_OR_MVN=-1;
  if(strcmp(argv[4],"CMN")==0)
  {
	DO_CMN_OR_MVN=0;
	strcpy(extensionName,"mfcATC0DAZC39");
  }
  else if(strcmp(argv[4],"MVNMA")==0)
  {
	DO_CMN_OR_MVN=1;
        if(DO_AVG==1)
	    strcpy(extensionName,"mfcATC0DAC39mva");
	else if(DO_AVG==2)
	    strcpy(extensionName,"mfcATC0DAC39mvacjp_atc");
  }
  else if(strcmp(argv[4],"NONE")==0)
  {
	DO_CMN_OR_MVN=2;
	strcpy(extensionName,"mfcATC0DAC39");
  }

  if(DO_CMN_OR_MVN==-1)
  {
	printf("Normalization參數不正確！請輸入CMN或MVNMA\n");
	exit(0);
  }
  
}

int main(int argc, char *argv[])
{
	FILE *fp_lst;
	char fileName[512], fname[1024];
	int t,d;
	FEATURE itriFEA;

	itriFEA.checkArgs(argc, argv);
        if( (fp_lst = fopen(argv[1], "rt"))==NULL ){
          printf("cant open LST file : %s\n", argv[1]);
          exit(0);
        }

	sprintf(IN_WAV_DIR,"%s",argv[2]);
	sprintf(OUT_C12_DIR,"%s/",argv[3]);
	sprintf(OUT_EC_DIR,"%s/",argv[3]);

	itriFEA.doInitial();
	while(fscanf(fp_lst, " %s\n",fileName)!=EOF)
	{
		sprintf(fname, "%s/%s", IN_WAV_DIR, fileName);
		// outMode: 0 => memory; 1 => output to disk file; 2 => display on screen 
		itriFEA.doExtract(fname, fileName, 1);

		/*** if outMode=0, get feature data with itriFEA.out_data[][] BEFORE itriFEA.doReset() ***/
		itriFEA.doReset();
	}

	if(fp_lst)
          fclose(fp_lst);
}

