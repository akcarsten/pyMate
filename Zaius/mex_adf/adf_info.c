/*=================================================================
 *
 * ADF_INFO.C	.MEX file to read adf information from ADF files
 *
 * The calling syntax is:
 *
 *  [nchan nobs sampt obslens adc2volts ...
 *                      nport portwidth] = adf_info(filename);
 *
 * To compile in Matlab:
 *  Make sure you have run "mex -setup"
 *  Then at the command prompt:
 *     >> mex adf_read.c adfapi.c adfwapi.c adfxapi.c
 *
 *  You should then adf_read function available
 *
 *=================================================================*/
/* $Revision: 1.0  $ */
/* $Revision: 1.01 $ 02-Jun-2000 YM/MPI : supports also adfw format */
/* $Revision: 1.02 $ 04-Apr-2001 YM/MPI : add output of obslengths  */
/* $Revision: 1.03 $ 26-Oct-2001 YM/MPI : supports new adfw API     */
/* $Revision: 1.04 $ 07-Oct-2002 YM/MPI : bug fix                   */
/* $Revision: 1.05 $ 12-Oct-2002 YM/MPI : warns unconverted file    */
/* $Revision: 1.10 $ 05-Dec-2012 YM/MPI : supports new adfx API     */
/* $Revision: 1.11 $ 07-Dec-2012 YM/MPI : supports adc2volts        */
/* $Revision: 1.12 $ 01-Mar-2013 YM/MPI : supports nport/portwidth  */
/* $Revision: 1.13 $ 23-Aug-2013 YM/MPI : adc2volts as double       */


#include <math.h>
#include <stdio.h>
#include "matrix.h"
#include "mex.h"
#include "adfapi.h"    /* adf  file format */
#include "adfwapi.h"   /* adfw file format */
#include "adfxapi.h"   /* adfx file format */


/* Input Arguments */
#define	FILE_IN	       prhs[0]

/* Output Arguments */
#define	ADF_NCHAN      plhs[0]
#define	ADF_NOBS       plhs[1]
#define ADF_SAMPT      plhs[2]
#define	ADF_OBSLENGTHS plhs[3]
#define ADF_ADC2VOLTS  plhs[4]
#define ADF_NPORT      plhs[5]
#define ADF_PORTWIDTH  plhs[6]


void mexFunction( int nlhs, mxArray *plhs[], 
                  int nrhs, const mxArray*prhs[] )
{ 
    FILE *fp;
    ADF_HEADER   *h = NULL;
    ADFW_HEADER *hw = NULL;
    ADFX_HEADER *hx = NULL;
    char *filename;
    int status, i, ftype;
    int nchan, nobs, nport;
    double sampt, *obslensptr, *adc2voltsptr, *portw, tmpv;

    /* Check for proper number of arguments */
    if (nrhs != 1) {
        mexPrintf("Usage: [nchan nobs sampt obslens adc2volts nport portwidth] = adf_info(filename)\n");
        mexPrintf("Notes: sampt in msec, obslens in pts,  ver.1.13 Aug-2013\n");
        return;
    }

    if (nlhs > 7) {
        mexErrMsgTxt("adf_info: Too many output arguments."); 
    }


    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_info: first arg must be a filename-string"); 
    }
    filename = mxArrayToString(FILE_IN);
    if (filename == NULL)
        mexErrMsgTxt("adf_info: not enough memory for the filename string");

    /* Open the file */
    fp = fopen(filename, "rb");
    if (!fp) {
        mexPrintf("adf_info: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_info: file not found."); 
    }
    /* check file format */
    ftype = adfx_getFileFormat(fp);
    switch (ftype) {
    case ADF_ADFX2013_UNCONV :
        mexPrintf("adf_info: file (ADFX format) not converted yet. '%s'\n",filename);
    case ADF_ADFX2013_CONV :
        //mexPrintf("adf_info: ADFX format\n");
        hx = adfx_readHeader(fp);
        if (hx == NULL) {
            fclose(fp);
            mexPrintf("adf_info: adxfile='%s'\n",filename);
            if (filename != NULL)  { mxFree(filename);  filename = NULL; }
            mexErrMsgTxt("adf_info: faild to read header");
        }
        nchan = hx->nchannels_ai;
        nobs  = hx->nobs;
        sampt = hx->us_per_sample/1000.;
        if (ftype == ADF_ADFX2013_UNCONV)  nobs = 0;
        break;
    case ADF_PCI6052E_UNCONV :
        mexPrintf("adf_info: file (16bitsAD) not converted yet. '%s'\n",filename);
    case ADF_PCI6052E_CONV :
        hw = adfw_readHeader(fp);
        if (hw == NULL) {
            fclose(fp);
            mexPrintf("adf_info: adffile='%s'\n",filename);
            if (filename != NULL)  { mxFree(filename);  filename = NULL; }
            mexErrMsgTxt("adf_info: faild to read header");
        }
        nchan = hw->nchannels;
        nobs  = hw->nobs;
        sampt = (double)hw->us_per_sample/1000.;  /* in milliseconds */
        if (ftype == ADF_PCI6052E_UNCONV)  nobs = 0;
        break;
    case ADF_WIN30_UNCONV :
        mexPrintf("adf_info: file (12bitsAD) not converted yet. '%s'\n",filename);
    case ADF_WIN30_CONV :
        h = adf_readHeader(fp);
        if (h == NULL) {
            fclose(fp);
            mexPrintf("adf_info: adffile='%s'\n",filename);
            if (filename != NULL)  { mxFree(filename);  filename = NULL; }
            mexErrMsgTxt("adf_info: faild to read header");
        }
        nchan = h->nchannels;
        nobs  = h->nobs;
        sampt = (double)h->us_per_sample/1000.;  /* in milliseconds */
        if (ftype == ADF_WIN30_UNCONV)  nobs = 0;
        break;
    default:
        fclose(fp);
        mexPrintf("adf_info: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_info: not adf/adfw file");
        break;
    }
    fclose(fp);

    ADF_NCHAN = mxCreateDoubleMatrix(1, 1, mxREAL);
    ADF_NOBS  = mxCreateDoubleMatrix(1, 1, mxREAL);
    ADF_SAMPT = mxCreateDoubleMatrix(1, 1, mxREAL);
    *mxGetPr(ADF_NCHAN) = (double)nchan;
    *mxGetPr(ADF_NOBS)  = (double)nobs;
    *mxGetPr(ADF_SAMPT) = sampt;

    // observation lengths
    if (nlhs > 3) {
        ADF_OBSLENGTHS = mxCreateDoubleMatrix(nobs, 1, mxREAL);
        obslensptr = mxGetPr(ADF_OBSLENGTHS);
        switch (ftype) {
        case ADF_ADFX2013_CONV :
            for (i = 0; i < nobs; i++)  *obslensptr++ = (double)hx->obscounts[i];
            break;
        case ADF_PCI6052E_CONV :
            for (i = 0; i < nobs; i++)  *obslensptr++ = (double)hw->obscounts[i];
            break;
        case ADF_WIN30_CONV :
            for (i = 0; i < nobs; i++)  *obslensptr++ = (double)h->obscounts[i];
            break;
        }
    }
    // adc2volts
    if (nlhs > 4) {
	  ADF_ADC2VOLTS  = mxCreateDoubleMatrix(nchan, 1, mxREAL);
	  adc2voltsptr   = mxGetPr(ADF_ADC2VOLTS);
        switch (ftype) {
        case ADF_ADFX2013_CONV :
        case ADF_ADFX2013_UNCONV :
            // multi-device, NI-DAQmx, each device has its own calibration.
            for (i = 0; i < nchan; i++) {
                *adc2voltsptr++ = hx->adc2volts[hx->ai_channels[i]];
            }
            break;
        case ADF_PCI6052E_CONV :
        case ADF_PCI6052E_UNCONV :
            // 16bit, legacy NI-DAQ
            if (hw->chan_gains[0] < 0)  tmpv = 0.5;
            else                        tmpv = (double)hw->chan_gains[0];
            tmpv = 10.0/65536.0/tmpv;
            for (i = 0; i < nchan; i++)  *adc2voltsptr++ = tmpv;
            break;
        case ADF_WIN30_CONV :
        case ADF_WIN30_UNCONV :
            // 12bit as -5V to 5V ????
            tmpv = 10.0/4096.0;
            for (i = 0; i < nchan; i++)  *adc2voltsptr++ = tmpv;
            break;
        }
    }
	// nport
	if (nlhs > 5) {
        ADF_NPORT = mxCreateDoubleMatrix(1, 1, mxREAL);
        nport = 0;
        switch (ftype) {
        case ADF_ADFX2013_CONV :
        case ADF_ADFX2013_UNCONV :
            nport = hx->nchannels_di;
            break;
        }
        *mxGetPr(ADF_NPORT) = (double)nport;
	}
    // port-width
    if (nlhs > 6) {
        ADF_PORTWIDTH = mxCreateDoubleMatrix(nport,1,mxREAL);
        portw = mxGetPr(ADF_PORTWIDTH);
        switch (ftype) {
        case ADF_ADFX2013_CONV :
        case ADF_ADFX2013_UNCONV :
            for (i = 0; i < nport; i++)  portw[i] = (double)adfx_getDiPortWidth(hx,i);
            break;
        }
    }
  
    if (h  != NULL)  adf_freeHeader(h);
    if (hw != NULL)  adfw_freeHeader(hw);
    if (hx != NULL)  adfx_freeHeader(hx);
  
    if (filename != NULL)  { mxFree(filename);  filename = NULL; }

    return;
}
