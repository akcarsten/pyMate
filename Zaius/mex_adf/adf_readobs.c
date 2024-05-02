/*=================================================================
 *
 * READADF.C	.MEX file to read adf observation periods from ADF
 *              files
 *
 * The calling syntax is:
 *
 *		[wv npts sampt] = adf_readobs(filename, obs, [start], [nsamples])
 *
 * To compile in Matlab:
 *  Make sure you have run "mex -setup"
 *  Then at the command prompt:
 *     >> mex adf_readobs.c adfapi.c adfwapi.c adfxapi.c
 *
 *  You should then adf_readobs function available
 *
 *=================================================================*/
/* $Revision: 1.0  $ 12-Apr-2001 YM/MPI */
/* $Revision: 1.01 $ 26-Oct-2001 YM/MPI : supports new adfw API    */
/* $Revision: 1.02 $ 05-Oct-2002 YM/MPI : supports partial reading */
/* $Revision: 1.03 $ 22-Oct-2002 YM/MPI : bug fix                  */
/* $Revision: 1.04 $ 06-Dec-2012 YM/MPI : supports new adfx API    */
/* $Revision: 1.05 $ 27-Aug-2013 YM/MPI : supports int32 Ai data   */

#include <math.h>
#include <stdio.h>
#include "matrix.h"
#include "mex.h"
#include "adfapi.h"    /* adf  file format */
#include "adfwapi.h"   /* adfw file format */
#include "adfxapi.h"   /* adfx file format */

/* Input Arguments */
#define	FILE_IN	     prhs[0]
#define	INDEX_IN	   prhs[1]
#define STARTINDX_IN prhs[2]
#define NSAMPLES_IN  prhs[3]

/* Output Arguments */
#define	ADF_OUT	     plhs[0]
#define	ADF_LENGTH	 plhs[1]
#define ADF_RATE     plhs[2]

static int resolution = 12;


void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
{
    FILE *fp = NULL;
    ADF_HEADER *h = NULL;
    ADFW_HEADER *hw = NULL;
    ADFX_HEADER *hx = NULL;
    char *filename;
    int obsindex, channel, startidx, nsamples;
    int status, i, npts, ftype, j, nchans;
    short *vals, *valsObs[64];
    double *dp, *p;
    double offset = (double) (1<<(resolution-1));
    char *aitype = NULL;
    int32_t *ivals;
    int64_t *lvals;


    /* Check for proper number of arguments */
    if (nrhs == 0) {
        mexPrintf("Usage: [wv npts sampt] = adf_readobs(filename,obs,[start],[nsamples])\n");
        mexPrintf("Notes: obs,start>=0, nsamples>=1       ver.1.05 Aug-2013\n");
        return;
    }
    if (nrhs < 2) { 
        mexErrMsgTxt("adf_readobs: Two input arguments required."); 
    } else if (nrhs == 3) {
        mexErrMsgTxt("adf_readobs: nsamples in pts required for partial reading.");
    } else if (nlhs > 4) {
        mexErrMsgTxt("adf_readobs: Too many output arguments."); 
    }

    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_readobs: first arg must be filename string"); 
    }
    i = (int)(mxGetM(FILE_IN) * mxGetN(FILE_IN)) + 1;
    filename = mxCalloc(i, sizeof(char));
    status = mxGetString(FILE_IN, filename, i);
    if (status != 0)
        mexWarnMsgTxt("adf_readobs: not enough space, filename string is truncated.");
  
    /* Get the obs index */
    obsindex = (int) mxGetScalar(INDEX_IN);
    /* Get start index/nsamples */
    if (nrhs > 2) {
        startidx = (int) mxGetScalar(STARTINDX_IN);
        nsamples = (int) mxGetScalar(NSAMPLES_IN);
        if (startidx < 0)   mexErrMsgTxt("adf_readobs: start index must be >= 0.");
        if (nsamples <= 0)  mexErrMsgTxt("adf_readobs: nsamples must be >= 1.");
    }

    /* open the file */
    fp = fopen(filename, "rb");
    if (!fp) {
        mexPrintf("adf_readobs: adffile='%s'\n",filename);
        mexErrMsgTxt("adf_readobs: file not found.");
    }

    /* check file format */
    ftype = adfx_getFileFormat(fp);
    switch (ftype) {
    case ADF_ADFX2013_CONV :
        hx = adfx_readHeader(fp);
        break;
    case ADF_PCI6052E_CONV :
        hw = adfw_readHeader(fp);
        break;
    case ADF_WIN30_CONV :
        h = adf_readHeader(fp);
        break;
    case ADF_ADFX2013_UNCONV :
    case ADF_PCI6052E_UNCONV :
    case ADF_WIN30_UNCONV :
        fclose(fp);
        mexPrintf("adf_readobs: adffile='%s'\n",filename);
        mexErrMsgTxt("adf_readobs: unconverted file");
        break;
    default:
        fclose(fp);
        mexPrintf("adf_readobs: adffile='%s'\n",filename);
        mexErrMsgTxt("adf_readobs: not adf/adfw/adfx file");
    }

    if (h == NULL && hw == NULL && hx == NULL) {
        fclose(fp);
        mexPrintf("adf_readobs: adffile='%s'\n",filename);
        mexErrMsgTxt("adf_readobs: invalid header info for adf/adfw/adfx");
    }

    /* read data */
    if (ftype ==  ADF_ADFX2013_CONV) {
        if (obsindex >= hx->nobs) {
            fclose(fp);  mexErrMsgTxt("adf_readobs: obs period out of range.");
        }
        nchans = hx->nchannels_ai;
        aitype = (char *)calloc(nchans,sizeof(char));
        if (nrhs < 3) {
            for (i = 0; i < nchans; i++)
                adfx_getObsPeriod(hx, fp, i, obsindex, &npts, &valsObs[i], &aitype[i]);
        } else {
            for (i = 0; i < nchans; i++)
                adfx_getObsPeriodPartial(hx, fp, i, obsindex, startidx, nsamples, &npts, &valsObs[i], &aitype[i]);
        }
        //printf("channeloffs=%d\n", hw->channeloffs[0]);
        //printf("obscounts=%d\n", hw->obscounts[0]);
        //printf("offsets=%d\n", hw->offsets[0]);
    } else if (ftype == ADF_PCI6052E_CONV) {
        if (obsindex >= hw->nobs) {
            fclose(fp);  mexErrMsgTxt("adf_readobs: obs period out of range.");
        }
        nchans = hw->nchannels;
        aitype = (char *)calloc(nchans,sizeof(char));
        for (i = 0; i < nchans; i++)  aitype[i] = 's';

        if (nrhs < 3) {
            for (i = 0; i < nchans; i++)
                adfw_getObsPeriod(hw, fp, i, obsindex, &npts, &valsObs[i]);
        } else {
            for (i = 0; i < nchans; i++)
                adfw_getObsPeriodPartial(hw, fp, i, obsindex, startidx, nsamples, &npts, &valsObs[i]);
        }
        //printf("channeloffs=%d\n", hw->channeloffs[0]);
        //printf("obscounts=%d\n", hw->obscounts[0]);
        //printf("offsets=%d\n", hw->offsets[0]);
    } else {
        // ftype == ADF_WIN30_CONV
        if (obsindex >= h->nobs) {
            fclose(fp);  mexErrMsgTxt("adf_readobs: obs period out of range.");
        }
        nchans = hw->nchannels;
        aitype = (char *)calloc(nchans,sizeof(char));
        for (i = 0; i < nchans; i++)  aitype[i] = 's';

        if (nrhs < 3) {
            for (i = 0; i < nchans; i++)
                adf_getObsPeriod(h, fp, i, obsindex, &npts, &valsObs[i]);
        } else {
            for (i = 0; i < nchans; i++)
                adf_getObsPeriodPartial(h, fp, i, obsindex, startidx, nsamples, &npts, &valsObs[i]);
        }
    }
    fclose(fp);

    /* Create a matrix for the return argument */ 
    ADF_OUT = mxCreateDoubleMatrix(npts, nchans, mxREAL);
    dp = mxGetPr(ADF_OUT);
    if (ftype == ADF_WIN30_CONV) {
        for (j = 0; j < nchans; j++) {
            vals = valsObs[j];
            for (p = dp, i = 0; i < npts; i++) *dp++ = vals[i]-offset;
        }
    } else {
        for (j = 0; j < nchans; j++) {
            switch (aitype[i]) {
            case 's':
                vals = (short *)valsObs[j];
                for (p = dp, i = 0; i < npts; i++) *dp++ = vals[i];
                break;
            case 'i':
                ivals = (int32_t *)valsObs[j];
                for (p = dp, i = 0; i < npts; i++) *dp++ = ivals[i];
                break;
            case 'l':
                lvals = (int64_t *)valsObs[j];
                for (p = dp, i = 0; i < npts; i++) *dp++ = lvals[i];
                break;
            }
        }
    }
    for (j = 0; j < nchans; j++)  free(valsObs[j]);

    if (nlhs > 1) {
        // read length
        ADF_LENGTH = mxCreateDoubleMatrix(1, 1, mxREAL);
        *mxGetPr(ADF_LENGTH) = (double) npts;
        // sampling time in msec
        if (nlhs > 2) {
            ADF_RATE = mxCreateDoubleMatrix(1, 1, mxREAL);
            if (ftype == ADF_ADFX2013_CONV) {
                *mxGetPr(ADF_RATE) = (double) hx->us_per_sample / 1000.;
            } else if (ftype == ADF_PCI6052E_CONV) {
                *mxGetPr(ADF_RATE) = (double) hw->us_per_sample / 1000.;
            } else {
                // ftype == ADF_WIN30_CONV
                *mxGetPr(ADF_RATE) = (double) h->us_per_sample /1000.;
            }
        }
    }

    if (h != NULL)   adf_freeHeader(h);
    if (hw != NULL)  adfw_freeHeader(hw);
    if (hx != NULL)  adfx_freeHeader(hx);

    if (aitype != NULL)  {  free(aitype);  aitype = NULL;  }

    return;
}
