/*=================================================================
 *
 * ADF_READ.C	.MEX file to read adf observation periods from ADF
 *              files
 *
 * The calling syntax is:
 *
 *		[wv npts sampt adc2volts] = adf_read(filename, obs, chan, [start],[nsamples],[datatype])
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
/* $Revision: 1.01 $ 02-Jun-2000 YM/MPI : supporting adfw format       */
/* $Revision: 1.02 $ 29-Mar-2001 YM/MPI : message for unconverted file */
/* $Revision: 1.03 $ 26-Oct-2001 YM/MPI : supports new adfw API        */
/* $Revision: 1.04 $ 05-Oct-2002 YM/MPI : supports partial reading     */
/* $Revision: 1.05 $ 24-May-2005 YM/MPI : supports "int" data type     */
/* $Revision: 1.06 $ 06-Jun-2009 YM/MPI : start/nsamples can be []     */
/* $Revision: 1.07 $ 18-Feb-2010 YM/MPI : bug fix of start/nsamples    */
/* $Revision: 1.10 $ 05-Dec-2012 YM/MPI : supports new adfx API        */
/* $Revision: 1.11 $ 07-Dec-2012 YM/MPI : supports adc2volts           */
/* $Revision: 1.12 $ 16-Jul-2013 YM/MPI : bug fix for -largeArrayDims  */
/* $Revision: 1.13 $ 23-Aug-2013 YM/MPI : adc2volts as double          */
/* $Revision: 1.14 $ 27-Aug-2013 YM/MPI : supports int32 Ai data       */


#include <math.h>
#include <stdio.h>
#include <string.h>
#include "matrix.h"
#include "mex.h"
#include "adfapi.h"    /* adf  file format */
#include "adfwapi.h"   /* adfw file format */
#include "adfxapi.h"   /* adfx file format */

/* Input Arguments */
#define	FILE_IN	       prhs[0]
#define	INDEX_IN       prhs[1]
#define	CHAN_IN	       prhs[2]
#define STARTINDX_IN   prhs[3]
#define NSAMPLES_IN    prhs[4]
#define DATATYPE_IN    prhs[5]

/* Output Arguments */
#define	ADF_OUT	       plhs[0]
#define	ADF_LENGTH	   plhs[1]
#define ADF_RATE       plhs[2]
#define ADF_ADC2VOLTS  plhs[3]

static int win30_resolution = 12;

#ifdef _WIN32
#  define STRICMP   _stricmp
#  define STRNICMP  _strnicmp
#else
#  define STRICMP   strcasecmp
#  define STRNICMP  strncasecmp
#endif

void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
{
    FILE *fp = NULL;
    ADF_HEADER   *h = NULL;
    ADFW_HEADER *hw = NULL;
    ADFX_HEADER *hx = NULL;
    char *filename;
    int obsindex, channel, startidx, nsamples;
    int status, i, npts, ftype;
    mxClassID datatype;
    short *vals;
    short win30_offset = (short) (1<<(win30_resolution-1));
    double tmpv;
    mwSize dims[2];
    char aitype;
    int32_t *ivals;
    int64_t *lvals;

    /* Check for proper number of arguments */
    if (nrhs == 0) {
        mexPrintf("Usage: [wv npts sampt adc2volts] = adf_read(filename,obs,chan,[start],[nsamples],[datatype])\n");
        mexPrintf("Notes: obs,chan,start>=0, nsamples>=1,  start,nsamples in pts, datatype as [double|single|int]\n");
        mexPrintf("                                       ver.1.14 Aug-2013\n");
        return;
    }
    if (nrhs < 3) { 
        mexErrMsgTxt("adf_read: Three input arguments required."); 
    } else if (nrhs == 4) {
        mexErrMsgTxt("adf_read: nsamples in pts required for partial reading.");
    } else if (nrhs > 6) {
        mexErrMsgTxt("adf_read: Too many input arguments."); 
    }

    if (nlhs > 4) {
        mexErrMsgTxt("adf_read: Too many output arguments."); 
    }

    /* Get the obs/channel index */
    obsindex = (int) mxGetScalar(INDEX_IN);
    channel  = (int) mxGetScalar(CHAN_IN);
    /* Get start index/nsamples */
    startidx = -1;
    nsamples = -1;
    if (nrhs > 3) {
        if (mxIsEmpty(STARTINDX_IN) == 0 && mxGetNumberOfElements(STARTINDX_IN) > 0) {
            startidx = (int) mxGetScalar(STARTINDX_IN);
            if (startidx < 0)   mexErrMsgTxt("adf_read: start index must be >= 0.");
        }
        if (mxIsEmpty(NSAMPLES_IN) == 0 && mxGetNumberOfElements(NSAMPLES_IN) > 0) {
            nsamples = (int) mxGetScalar(NSAMPLES_IN);
            if (nsamples <= 0)  mexErrMsgTxt("adf_read: read nsamples must be >= 1.");
        }
    }
    /* Get data type */
    datatype = mxDOUBLE_CLASS;
    if (nrhs > 5) {
        if (mxIsChar(DATATYPE_IN) != 1) {
            mexErrMsgTxt("adf_read: 6th arg must be datatype string [double|single(float)|int]"); 
        }
        if (mxIsEmpty(DATATYPE_IN) == 0 && mxGetNumberOfElements(DATATYPE_IN) > 1) {
            char datastr[16];
            status = mxGetString(DATATYPE_IN,datastr,12);
            //printf("%s", datastr);
            if (STRNICMP(datastr,"int",3) == 0 || STRICMP(datastr,"short") == 0) {
                datatype = mxINT16_CLASS;
            } else if (STRICMP(datastr,"double") == 0) {
                datatype = mxDOUBLE_CLASS;
            } else if (STRICMP(datastr,"float") == 0 || STRICMP(datastr,"single") == 0) {
                datatype = mxSINGLE_CLASS;
            } else {
                mexErrMsgTxt("adf_read: 6th arg must be datatype string [double|single(float)|int]"); 
            }
        }
    }


    /* Get the filename */
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_read: first arg must be a filename-string"); 
    }
    filename = mxArrayToString(FILE_IN);
    if (filename == NULL)
        mexErrMsgTxt("adf_read: not enough memory for the filename string");

    /* open the file */
    fp = fopen(filename, "rb");
    if (!fp) {
        mexPrintf("adf_read: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_read: file not found.");
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
    case ADF_WIN30_UNCONV :
    case ADF_PCI6052E_UNCONV :
    case ADF_ADFX2013_UNCONV :
        fclose(fp);
        mexPrintf("adf_read: adffile='%s'\n",filename);
        mexErrMsgTxt("adf_read: unconverted file");
        break;
    default:
        fclose(fp);
        mexPrintf("adf_read: adffile='%s'\n",filename);
        mexErrMsgTxt("adf_read: not adf/adfw file");
    }

    if (h == NULL && hw == NULL && hx == NULL) {
        fclose(fp);
        mexPrintf("adf_read: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_read: invalid header info for adf/adfw");
        return;
    }

    /* read data */
    switch (ftype) {
    case ADF_ADFX2013_CONV :
        if (startidx < 0) {
            status = adfx_getObsPeriod(hx, fp, channel, obsindex, &npts, &vals, &aitype);
        } else {
            status = adfx_getObsPeriodPartial(hx, fp, channel, obsindex, startidx, nsamples, &npts, &vals, &aitype);
        }
        // upgrade integer class, if needed
        if (datatype == mxINT16_CLASS) {
            if (aitype == 'i')       datatype = mxINT32_CLASS;
            else if (aitype == 'l')  datatype = mxINT64_CLASS;
        }
        break;
    case ADF_PCI6052E_CONV :
        aitype = 's';
        if (startidx < 0) {
            status = adfw_getObsPeriod(hw, fp, channel, obsindex, &npts, &vals);
        } else {
            status = adfw_getObsPeriodPartial(hw, fp, channel, obsindex, startidx, nsamples, &npts, &vals);
        }
        //printf("channeloffs=%d\n", hw->channeloffs[0]);
        //printf("obscounts=%d\n", hw->obscounts[0]);
        //printf("offsets=%d\n", hw->offsets[0]);
        break;
    case ADF_WIN30_CONV :
        aitype = 's';
        if (startidx < 0) {
            status = adf_getObsPeriod(h, fp, channel, obsindex, &npts, &vals);
        } else {
            status = adf_getObsPeriodPartial(h, fp, channel, obsindex, startidx, nsamples, &npts, &vals);
        }
        break;
    }
    fclose(fp);  fp = NULL;

    if (status < 0) {
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        if (status == -1)  mexErrMsgTxt("adf_read: channel out of range.");
        if (status == -2)  mexErrMsgTxt("adf_read: obs period out of range.");
        if (status == -3)  mexErrMsgTxt("adf_read: start/nsamples out of range.");
        return;
    }

    /* Create a matrix for the return argument */
    if (ftype == ADF_WIN30_CONV) {
        for (i = 0; i < npts; i++)  vals[i] = vals[i] - win30_offset;
    }
    dims[0] = 1;  dims[1] = npts;
    if (datatype == mxDOUBLE_CLASS) {
        double *p;
        ADF_OUT = mxCreateDoubleMatrix(1, npts, mxREAL);
        switch (aitype) {
        case 's':
            for (p = (double *)mxGetData(ADF_OUT), i = 0; i < npts; i++) *p++ = (double)vals[i];
            break;
        case 'i':
            ivals = (int32_t *)vals;
            for (p = (double *)mxGetData(ADF_OUT), i = 0; i < npts; i++) *p++ = (double)ivals[i];
            break;
        case 'l':
            lvals = (int64_t *)vals;
            for (p = (double *)mxGetData(ADF_OUT), i = 0; i < npts; i++) *p++ = (double)lvals[i];
            break;
        }
    } else if (datatype == mxINT16_CLASS) {
        ADF_OUT = mxCreateNumericArray(2, dims, mxINT16_CLASS, mxREAL);
        memcpy(mxGetData(ADF_OUT), vals, npts*sizeof(short));
    } else if (datatype == mxINT32_CLASS) {
        ADF_OUT = mxCreateNumericArray(2, dims, mxINT32_CLASS, mxREAL);
        memcpy(mxGetData(ADF_OUT), vals, npts*sizeof(int32_t));
    } else if (datatype == mxINT64_CLASS) {
        ADF_OUT = mxCreateNumericArray(2, dims, mxINT64_CLASS, mxREAL);
        memcpy(mxGetData(ADF_OUT), vals, npts*sizeof(int64_t));
    } else if (datatype == mxSINGLE_CLASS) {
        float *p;
        ADF_OUT = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
        switch (aitype) {
        case 's':
            for (p = (float  *)mxGetData(ADF_OUT), i = 0; i < npts; i++) *p++ = (float)vals[i];
            break;
        case 'i':
            ivals = (int32_t *)vals;
            for (p = (float *)mxGetData(ADF_OUT), i = 0; i < npts; i++) *p++ = (float)ivals[i];
            break;
        case 'l':
            lvals = (int64_t *)vals;
            for (p = (float *)mxGetData(ADF_OUT), i = 0; i < npts; i++) *p++ = (float)lvals[i];
            break;
        }
    }
    free(vals);

    // read length
    if (nlhs > 1) {
        ADF_LENGTH = mxCreateDoubleMatrix(1, 1, mxREAL);
        *mxGetPr(ADF_LENGTH) = (double) npts;
    }
    // sampling time in msec
    if (nlhs > 2) {
        ADF_RATE = mxCreateDoubleMatrix(1, 1, mxREAL);
        switch (ftype) {
        case ADF_ADFX2013_CONV :
            *mxGetPr(ADF_RATE) =  hx->us_per_sample / 1000.;
            break;
        case ADF_PCI6052E_CONV :
            *mxGetPr(ADF_RATE) = (double) hw->us_per_sample / 1000.;
            break;
        case ADF_WIN30_CONV :
            *mxGetPr(ADF_RATE) = (double) h->us_per_sample /1000.;
            break;
        }
    }
    // adc2volts
    if (nlhs > 3) {
        ADF_ADC2VOLTS = mxCreateDoubleMatrix(1, 1, mxREAL);
        switch (ftype) {
        case ADF_ADFX2013_CONV :
            // multi-device, NI-DAQmx, each device has its own calibration.
            tmpv = hx->adc2volts[hx->ai_channels[channel]];
            break;
        case ADF_PCI6052E_CONV :
            // 16bit, legacy NI-DAQ
            if (hw->chan_gains[0] < 0)  tmpv = 0.5;
            else                        tmpv = (double)hw->chan_gains[0];
            tmpv = 10.0/65536.0/tmpv;
            break;
        case ADF_WIN30_CONV :
            // 12bit as -5V to 5V ????
            tmpv = 10.0/4096.0;
            break;
        }
        *mxGetPr(ADF_ADC2VOLTS) = tmpv;
    }

    if (h  != NULL)  adf_freeHeader(h);
    if (hw != NULL)  adfw_freeHeader(hw);
    if (hx != NULL)  adfx_freeHeader(hx);

    if (filename != NULL)  { mxFree(filename);  filename = NULL; }

    return;
}
