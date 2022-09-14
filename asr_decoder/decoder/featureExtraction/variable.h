
extern double HW[FRAME_SIZE];
extern int gb_bank[FFT_ORDER_2];
extern int    gb_totalFrames;
extern double gb_band_eng[MAX_FRM_NO][FILTERNO+1];
extern float  gb_eng[MAX_FRM_NO],gb_deng[MAX_FRM_NO],gb_ddeng[MAX_FRM_NO];
extern double gb_fft_wt[FFT_ORDER_2];

extern CEP_DATA *gb_cep_data;
/***   initial model parameters  ***/
/*
extern float     gb_ui[I_MODEL][I_STATE][I_MIX][INPUT], gb_vi[I_MODEL][I_STATE][I_MIX][INPUT],
                 gb_di[I_MODEL][I_STATE][I_MIX],      gb_ci[I_MODEL][I_STATE][I_MIX],
                 gb_ai[I_MODEL][I_STATE][I_STATE],	  gb_i_trout[I_MODEL];
*/
/***   final model parameters  ***/
/*
extern float     gb_uf[F_MODEL][F_STATE][F_MIX][INPUT], gb_vf[F_MODEL][F_STATE][F_MIX][INPUT],
                 gb_df[F_MODEL][F_STATE][F_MIX],      gb_cf[F_MODEL][F_STATE][F_MIX],
                 gb_af[F_MODEL][F_STATE][F_STATE],	  gb_f_trout[F_MODEL];
*/
/*   silence model parameters  */
/*
extern float     gb_us[S_MIX+1][INPUT], gb_vs[S_MIX+1][INPUT],
                 gb_ds[S_MIX+1],      gb_cs[S_MIX+1];
*/
/*  global variables  */
/*
extern int       gb_i_mix_num[I_MODEL], gb_f_mix_num[F_MODEL];

extern int       gb_topnIndex[TOPN];
extern float    gb_topnScore[TOPN];
*/
