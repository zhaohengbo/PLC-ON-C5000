#ifndef _DSPLIB
#define _DSPLIB

#include "tms320.h"

/* 16-bit fft */

short cfft8 (DATA *x, DATA scale);
short cfft16 (DATA *x, DATA scale);
short cfft32 (DATA *x, DATA scale);
short cfft64 (DATA *x, DATA scale);
short cfft128 (DATA *x, DATA scale);
short cfft256 (DATA *x, DATA scale);
short cfft512 (DATA *x, DATA scale);
short cfft1024 (DATA *x, DATA scale);

short rfft16 (DATA *x, DATA scale);
short rfft32 (DATA *x, DATA scale);
short rfft64 (DATA *x, DATA scale);
short rfft128 (DATA *x, DATA scale);
short rfft256 (DATA *x, DATA scale);
short rfft512 (DATA *x, DATA scale);
short rfft1024 (DATA *x, DATA scale);

/* 32-bit fft */

short cfft32_8 (LDATA *x, DATA scale);
short cfft32_16 (LDATA *x, DATA scale);
short cfft32_32 (LDATA *x, DATA scale);
short cfft32_64 (LDATA *x, DATA scale);
short cfft32_128 (LDATA *x, DATA scale);
short cfft32_256 (LDATA *x, DATA scale);
short cfft32_512 (LDATA *x, DATA scale);
short cfft32_1024 (LDATA *x, DATA scale);


/* 16-bit ifft */

short cifft8 (DATA *x, DATA scale);
short cifft16 (DATA *x, DATA scale);
short cifft32 (DATA *x, DATA scale);
short cifft64 (DATA *x, DATA scale);
short cifft128 (DATA *x, DATA scale);
short cifft256 (DATA *x, DATA scale);
short cifft512 (DATA *x, DATA scale);
short cifft1024 (DATA *x, DATA scale);

short rifft16 (DATA *x, DATA scale);
short rifft32 (DATA *x, DATA scale);
short rifft64 (DATA *x, DATA scale);
short rifft128 (DATA *x, DATA scale);
short rifft256 (DATA *x, DATA scale);
short rifft512 (DATA *x, DATA scale);
short rifft1024 (DATA *x, DATA scale);

short unpacki_16(DATA *x);
short unpacki_32(DATA *x);
short unpacki_64(DATA *x);
short unpacki_128(DATA *x);
short unpacki_256(DATA *x);
short unpacki_512(DATA *x);
short unpacki_1024(DATA *x);

/* 32-bit Ifft */

short cifft32_8 (LDATA *x, DATA scale);
short cifft32_16 (LDATA *x, DATA scale);
short cifft32_32 (LDATA *x, DATA scale);
short cifft32_64 (LDATA *x, DATA scale);
short cifft32_128 (LDATA *x, DATA scale);
short cifft32_256 (LDATA *x, DATA scale);
short cifft32_512 (LDATA *x, DATA scale);
short cifft32_1024 (LDATA *x, DATA scale);

short cbrev (DATA *x, DATA *y, ushort n);
short cbrev32 (LDATA *x, LDATA *y, ushort n);


/* correlations */

short acorr_raw (DATA *x, DATA *r, ushort nx, ushort nr);
short acorr_bias(DATA *x, DATA *r, ushort nx, ushort nr);
short acorr_unbias(DATA *x, DATA *r, ushort nx, ushort nr);

short corr_raw	(DATA *x, DATA *y, DATA *r, ushort nx, ushort ny);
short corr_bias  (DATA *x, DATA *y, DATA *r, ushort nx, ushort ny);
short corr_unbias  (DATA *x, DATA *y, DATA *r, ushort nx, ushort ny);

/* filtering and convolution */

short convol (DATA *x, DATA *y, DATA *r, ushort ny, ushort nr);
short fir(DATA *x, DATA *h, DATA *r,DATA **d, ushort nh, ushort nx);
short firs(DATA *x, DATA *r,DATA **d, ushort nh, ushort nx);
short firs2(DATA *x, DATA *h, DATA *r,DATA **d, ushort nh, ushort nx);
short cfir(DATA *x, DATA *h, DATA *r,DATA **d, ushort nh, ushort nx);

short iircas4(DATA *x,DATA *h,DATA *r,DATA **d, ushort nbiq, ushort nx);
short iircas5(DATA *x,DATA *h,DATA *r,DATA **d, ushort nbiq, ushort nx);
short iircas51(DATA *x,DATA *h,DATA *r,DATA **d, ushort nbiq, ushort nx);
short iir32(DATA *x,LDATA *h,DATA *r,LDATA **d, ushort nbiq, ushort nx);

short firdec(DATA *x, DATA *h, DATA *r,DATA **d, ushort nh, ushort nx, ushort D);
short firinterp(DATA *x,DATA *h,DATA *r,DATA **db,ushort nh,ushort nx,ushort I);

short latfor (DATA *x, DATA *h, DATA *r, DATA *d, ushort nx, ushort nh);

/* adaptive filtering */

short dlms(DATA *x,DATA *h,DATA *r, DATA **d, DATA *des, DATA step, ushort nh, ushort nx);
short nblms (DATA *x,DATA *h,DATA *r, DATA **d, DATA *des, ushort nh, ushort nx, ushort nb, DATA **norm_e, int l_tau, int cutoff, int gain);
short ndlms (DATA *x, DATA *h, DATA *r, DATA *d, DATA *des, ushort nh, ushort nx, int l_tau, int cutoff, int gain, DATA *norm_d);

/* math */

short add (DATA *x, DATA *y, DATA *r, ushort nx, short scale);
short sub(DATA *x, DATA *y,  DATA *r, ushort nx, ushort scale);
short neg(DATA *x, DATA *r, ushort nx);

short ldiv16(LDATA *x, DATA *y, DATA *z, DATA *exp, unsigned short nx);
void recip16 (DATA *x, DATA *z, DATA *zexp, ushort n);


short expn(DATA *x, DATA *r, ushort nx);
short logn(DATA *x, LDATA *r, ushort nx);
short log_2(DATA *x, LDATA *r, ushort nx);
short log_10(DATA *x, LDATA *r, ushort nx);

short sqrt_16(DATA *x, DATA *y, short nx);

short maxidx (DATA *x, ushort nx);
short maxval (DATA *x, ushort nx);
short minidx (DATA *x, ushort nx);
short minval (DATA *x, ushort nx);

short rand16(DATA *r, ushort nr);
void rand16init(void);

short mul32(LDATA *x, LDATA *y,  LDATA *r, ushort nx);
short neg32(LDATA *x, LDATA *r, ushort nx);
short power(DATA *x, LDATA *r, ushort nx);

/* matrix */

short mmul(DATA *x1,short row1,short col1,DATA *x2,short row2,short col2,DATA *r);
short mtrans(DATA *x, short row, short col, DATA *r);

/* trigonometric */

short sine (DATA *x, DATA *r, ushort nx);

/* miscellaneous */

short fltoq15(float *x, DATA *r, ushort nx);
short  q15tofl (DATA *x, float *r, ushort nx);


/* macro definition */

#define acorr(n1, n2, n3, n4, type) acorr_##type(n1, n2, n3, n4)
#define corr(n1, n2, n3, n4, n5, type) corr_##type(n1, n2, n3, n4, n5)
#define dummy(x,n,scale)	cfft##n(x,scale)	// macro for generic FFT look
#define cfft(x,n,scale)		dummy(x,n,scale)

#define dummy2(x,n,scale)	rfft##n(x,scale)	// macro for generic FFT look
#define rfft(x,n,scale)		dummy2(x,n,scale)

#define dummy3(x,n,scale)	cifft##n(x,scale)	// macro for generic IFFT look
#define cifft(x,n,scale)	dummy3(x,n,scale)

#define dummy4(x,n,scale)	rifft##n(x,scale)	// macro for generic IFFT look
#define rifft(x,n,scale)	dummy4(x,n,scale)

#define dummy5(x,n,scale)	cfft32_##n(x,scale)	// macro for generic FFT look
#define cfft_32(x,n,scale)	dummy5(x,n,scale)

#define dummy6(x,n,scale)	cifft32_##n(x,scale)	// macro for generic FFT look
#define cifft_32(x,n,scale)	dummy6(x,n,scale)

#define dummy7(x,n)	        unpacki_##n(x)	     // macro for generic unpacki look
#define unpacki(x,n)	    dummy7(x,n)

#endif
