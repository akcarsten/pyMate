/*
 *	cnvadfw.c
 *
 *	ver 1.00 Jul-2000     YM  extended from cnvadf.c
 *	ver 1.01 08-Aug-2000  YM  avoids overwrite of datafile
 *	ver 1.02 28-Mar-2000  YM  supports Matlab
 *	ver 1.03 26-Oct-2001  YM  supports command-line threshold (adfw only)
 *                            new adfw API supported
 *	ver 1.04 07-Oct-2002  YM  new adfw API supported
 *
 *  To compile in Matlab:
 *    Make sure you have run "mex -setup"
 *    Then at the command prompt:
 *    >> mex cnvadfw.c adfapi.c adfwapi.c -D_USE_IN_MATLAB
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

//#define _USE_IN_MATLAB             /* use in matlab or not */
#ifdef _USE_IN_MATLAB
#  include "matrix.h"
#  include "mex.h"
   /* Input Arguments */
#  define FILE_IN   prhs[0]        /* raw file             */
#  define FILE_OUT  prhs[1]        /* conveted file        */
#else
#include "cnvadfw.h"
//#  define _MAKE_TEST_FILE          /* make a test file     */
#  ifdef _MAKE_TEST_FILE
#    define MAX_READ_BUFSIZE (100000) /* will be multiplied by nchannels */
#  endif
#endif

#include "adfapi.h"
#include "adfwapi.h"

#ifdef _WIN32
#  define STRCMP   _stricmp
#else
#  define STRCMP   strcmp
#endif


/*************************************************/
/* prototypes */
int  convADF(int argc, char *argv[]);
int  convADFW(int argc, char *argv[]);
void printinfo(char *str);
void makeTestData(short *data, int nChan, int n, int bias);
char msgbuf[64];

/*************************************************/
int convADF(int argc, char *argv[])
{
	FILE *fp;
	ADF_HEADER *h;
	ADF_DIR *d;

	fp = fopen(argv[1], "rb");
	if (!fp) {
    sprintf(msgbuf, "%s: unable to open file %s\n", argv[1]);
    printinfo(msgbuf);
		return -1;
	}

	h = adf_readHeader(fp);
	if (!h) {
		sprintf(msgbuf, "%s: unable to read adf file %s\n", argv[0], argv[1]);
    printinfo(msgbuf);
		return 1;
	}

	if (h->nobs == 0) {		/* Needs to be converted */
		FILE *ofp;
		if (argc < 3) ofp = stdout;
		else ofp = fopen(argv[2], "wb");
		if (!ofp) {
			sprintf(msgbuf, "%s: error opening output file\n", argv[0]);
			printinfo(msgbuf);
			adf_freeHeader(h);
			fclose(fp);
			return 1;
		}
		d = adf_createDirectory(fp, h);
		adf_convertFile(h, d, fp, ofp);
		if (ofp != stdout) fclose(ofp);
	} else {
		adf_printInfo(h, fp);
	}

	adf_freeHeader(h);
	fclose(fp);

  return 0;
}

int convADFW(int argc, char *argv[])
{
	FILE *fp;
	ADFW_HEADER *h;
	ADFW_DIR *d;
	short logicH, logicL;

	logicH = DAQ_ANALOG_HIGH;  logicL = DAQ_ANALOG_LOW;

	if (argc >= 4) {
		logicH = (short)atoi(argv[3]);
		if (argc >= 5)  logicL = (short)atoi(argv[4]);
		else            logicL = logicH;
	}

	fp = fopen(argv[1], "rb");
	if (!fp) {
		sprintf(msgbuf, "%s: unable to open file %s\n", argv[1]);
    printinfo(msgbuf);
		return -1;
	}

	h = adfw_readHeader(fp);
	if (!h) {
		sprintf(msgbuf, "%s: unable to read adf file %s\n", argv[0], argv[1]);
		printinfo(msgbuf);
		return 1;
	}

	if (h->nobs == 0) {		/* Needs to be converted */
		FILE *ofp;
		if (argc < 3) ofp = stdout;
		else ofp = fopen(argv[2], "wb");
		if (!ofp) {
			sprintf(msgbuf, "%s: error opening output file\n", argv[0]);
			printinfo(msgbuf);
			adfw_freeHeader(h);
			fclose(fp);
			return 1;
		}
		d = adfw_createDirectoryEx(fp, h, logicH, logicL);
		adfw_convertFile(h, d, fp, ofp);
		if (ofp != stdout) fclose(ofp);
	} else {
		adfw_printInfo(h, fp);
	}

	adfw_freeHeader(h);
	fclose(fp);

  return 0;
}

void printinfo(char *str)
{
#ifdef _USE_IN_MATLAB
	mexPrintf(str);
#else
	fprintf(stderr,"%s",str);
#endif
}

void makeTestData(short *data, int nChan, int n, int bias)
{
	int i, ch;

	for (i = 0; i < n; i+=nChan) {
		for (ch = 0; ch < nChan - 1; ch++)		data[i + ch] = ch+bias;
		data[i + nChan - 1] = DAQ_ANALOG_HIGH+1;
	}
	for (i = 0; i < 10*nChan; i+=nChan)	data[i+nChan-1] = DAQ_ANALOG_LOW-1;
	for (i = n-10*nChan; i < n; i+=nChan)	data[i+nChan-1] = DAQ_ANALOG_LOW-1;
}


/* main body ************************************************/
#ifdef _USE_IN_MATLAB
void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
{
	int ftype, status;
	int argc, buflen;
	char *argv[5], logicV[2][64];

	/* mimick main(argc,argv) */
	argc = nrhs + 1;
	argv[0] = "cnvadfw";  argv[1] = "";  argv[2] = "";
	argv[3] = logicV[0];  argv[4] = logicV[1];;

	if (nrhs < 2) {
		mexPrintf("Usage: cnvadfw(rawfile,convfile,[logicH],[logicL])\n");
		mexPrintf("Notes:                           ver.1.04 Mar-2013\n");
		return;
	}
	/* Check the filename(s) */
	if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
		mexErrMsgTxt("cnvadfw: first arg must be filename string"); 
	}
	if (nrhs > 1) {
		if (mxIsChar(FILE_OUT) != 1 || mxGetM(FILE_OUT) != 1) {
			mexErrMsgTxt("cnvadfw: second arg must be filename string"); 
		}
	}

	/* Get the filename(s) */
	buflen = (mxGetM(FILE_IN) * mxGetN(FILE_IN)) + 1;
	argv[1] = mxCalloc(buflen,sizeof(char));
	status = mxGetString(FILE_IN, argv[1], buflen);
	if (status != 0)
		mexWarnMsgTxt("cnvadfw: not enough space, filename string is truncated.");
	if (argc >= 3) {
		buflen = (mxGetM(FILE_OUT) * mxGetN(FILE_OUT)) + 1;
		argv[2] = mxCalloc(buflen,sizeof(char));
		status = mxGetString(FILE_OUT, argv[2], buflen);
		if (status != 0)
			mexWarnMsgTxt("cnvadfw: not enough space, filename string is truncated.");
		/* Check overwrite */
		if (STRCMP(argv[1],argv[2]) == 0)
			mexErrMsgTxt("cnvadfw: error, rawfile = convfile");
		if (argc >= 4) {
			sprintf(argv[3],"%d",(int)mxGetScalar(prhs[2]));
			if (argc >= 5) {
				sprintf(argv[4],"%d",(int)mxGetScalar(prhs[3]));
			}
			//sprintf(msgbuf,"%s %s\n",argv[3],argv[4]);
			//mexPrintf(msgbuf);
		}
	}

	ftype = adfw_checkFileFormat(argv[1]);
	//sprintf(msgbuf,"%s %s %d\n",argv[1],argv[2],ftype);
	//mexPrintf(msgbuf);

	switch (ftype) {
	case -1:
	  sprintf(msgbuf,"cnvadfw: failed to open '%s'\n",argv[1]);
	  mexErrMsgTxt(msgbuf); 
		break;  
	case ADF_WIN30_UNCONV :
		status = convADF(argc,argv);	break;
	case ADF_PCI6052E_UNCONV :
		status = convADFW(argc,argv);	break;
	case ADF_WIN30_CONV :
	case ADF_PCI6052E_CONV :
		mexWarnMsgTxt("cnvadfw: already converted");
		break;  
	default:
		mexWarnMsgTxt("cnvadfw: unknown file type");
		break;
	}

	return;
}
#else  // else of _USE_IN_MATLAB
#  ifndef _MAKE_TEST_FILE
int main(int argc, char *argv[])
{
	int ftype, status;

	if (argc < 2) {
		fprintf(stderr, "usage: %s rawfile convfile\n", argv[0]);
		exit(0);
	} else if (argc == 3) {
		if (STRCMP(argv[1],argv[2]) == 0) {
			fprintf(stderr, "%s error: rawfile = convfile\n", argv[0]);
			exit(0);
		}
	}

	ftype = adfw_checkFileFormat(argv[1]);

	switch (ftype) {
	case ADF_WIN30_UNCONV :
		status = convADF(argc,argv);	break;
	case ADF_PCI6052E_UNCONV :
		status = convADFW(argc,argv);	break;
	case ADF_WIN30_CONV :
	case ADF_PCI6052E_CONV :
		fprintf(stderr, "%s warning: %s is already converted.\n",argv[0],argv[1]);
		status = 1;
		break;
	default:
		fprintf(stderr, "%s error: %s is unknown file format.\n",argv[0],argv[1]);
		status = 0;
		break;
	}

	return status;

}

#  else  // else of !_MAKE_TEST_FILE
/*************************************************/
int main(int argc, char *argv[])
{
	char buf[256];
	ADFW_HEADER *h;
	FILE *fp;
	short *samples;
	int n, nChan = 16;

	if (argc < 2) {
		fprintf(stderr, "usage: %s filename\n", argv[0]);
		exit(0);
	}


	h = (ADFW_HEADER *)buf;
	adfw_initHeader(h, 0);
	h->nchannels = nChan;

	samples = (short *) calloc(MAX_READ_BUFSIZE*3*nChan, sizeof(short));
	if (!samples)	return 0;

	fp = fopen(argv[1], "wb");
	if (!fp) {
		fprintf(stderr, "%s: unable to open file %s\n", argv[1]);
		return -1;
	}
	printf("\n %s : ",argv[1]);

	// write a header
	fwrite(buf,sizeof(char),ADFW_HEADER_SIZE,fp);
	printf(" header..");

	// write data
	// n < MAX_READ_BUFSIZE
	n = MAX_READ_BUFSIZE/2;
	makeTestData(samples,nChan,n*nChan,0);
	fwrite(samples,sizeof(short),n*nChan,fp);
	printf(" obs1..%d",n);
	// n = MAX_READ_BUFSIZE * N
	n = MAX_READ_BUFSIZE*2;
	makeTestData(samples,nChan,n*nChan,10);
	fwrite(samples,sizeof(short),n*nChan,fp);
	printf(" obs2..%d",n);
	printf(" %d ", samples[n*nChan - 2]);
	// n > MAX_READ_BUFSIZE * N;
	n = MAX_READ_BUFSIZE*2 + 100;
	makeTestData(samples,nChan,n*nChan,100);
	fwrite(samples,sizeof(short),n*nChan,fp);
	printf(" obs3..%d",n);
	printf(" %d ", samples[n*nChan - 2]);

	fclose(fp);
	free(samples);

	printf(" done\n");


	return 1;
}

#  endif // end of _MAKE_TEST_FILE
#endif // end of _USE_IN_MATLAB
