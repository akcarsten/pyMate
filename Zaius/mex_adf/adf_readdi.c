/*=================================================================
 *
 * ADF_READDI.C	.MEX file to read adf observation periods from ADF
 *              files
 *
 * The calling syntax is:
 *
 *  [patt npts sampt portwidth] = adf_readdi(filename, obs, port, [start],[nsamples])
 *
 * To compile in Matlab:
 *  Make sure you have run "mex -setup"
 *  Then at the command prompt:
 *     >> mex adf_readdi.c adfapi.c adfwapi.c adfxapi.c
 *
 *  You should then adf_readdi function available
 *
 *=================================================================*/
/* $Revision: 1.00 $ 01-Mar-2013 YM/MPI :                              */
/* $Revision: 1.01 $ 16-Jul-2013 YM/MPI : bug fix for -largeArrayDims  */


#include <math.h>
#include <stdio.h>
#include "matrix.h"
#include "mex.h"
#include "adfxapi.h"   /* adfx file format */

/* Input Arguments */
#define	FILE_IN	       prhs[0]
#define	INDEX_IN       prhs[1]
#define	PORT_IN	       prhs[2]
#define STARTINDX_IN   prhs[3]
#define NSAMPLES_IN    prhs[4]

/* Output Arguments */
#define	ADF_OUT	       plhs[0]
#define	ADF_LENGTH	   plhs[1]
#define ADF_RATE       plhs[2]
#define ADF_PORTWIDTH  plhs[3]


void mexFunction( int nlhs, mxArray *plhs[], 
		  int nrhs, const mxArray*prhs[] )
{
    FILE *fp = NULL;
    ADFX_HEADER *hx = NULL;
    char *filename;
    int obsindex, iport, startidx, nsamples, portw;
    int status, i, npts, ftype;
    void *vals;
    mwSize dims[2];
    char ditype;


    // Check for proper number of arguments
    if (nrhs == 0) {
        mexPrintf("Usage: [patt npts sampt portwidth] = adf_readdi(filename,obs,port,[start],[nsamples])\n");
        mexPrintf("Notes: obs,port,start>=0, nsamples>=1,  start,nsamples in pts\n");
        mexPrintf("                                       ver.1.01 Jul-2013\n");
        return;
    }
    if (nrhs < 3) { 
        mexErrMsgTxt("adf_readdi: Three input arguments required."); 
    } else if (nrhs == 4) {
        mexErrMsgTxt("adf_readdi: nsamples in pts required for partial reading.");
    } else if (nrhs > 5) {
        mexErrMsgTxt("adf_readdi: Too many input arguments."); 
    }

    if (nlhs > 4) {
        mexErrMsgTxt("adf_readdi: Too many output arguments."); 
    }


    // Get the obs/port index
    obsindex = (int) mxGetScalar(INDEX_IN);
    iport    = (int) mxGetScalar(PORT_IN);
    // Get start index/nsamples
    startidx = -1;
    nsamples = -1;
    if (nrhs > 3) {
        if (mxIsEmpty(STARTINDX_IN) == 0 && mxGetNumberOfElements(STARTINDX_IN) > 0) {
            startidx = (int) mxGetScalar(STARTINDX_IN);
            if (startidx < 0)   mexErrMsgTxt("adf_readdi: start index must be >= 0.");
        }
        if (mxIsEmpty(NSAMPLES_IN) == 0 && mxGetNumberOfElements(NSAMPLES_IN) > 0) {
            nsamples = (int) mxGetScalar(NSAMPLES_IN);
            if (nsamples <= 0)  mexErrMsgTxt("adf_readdi: read nsamples must be >= 1.");
        }
    }

    // Get the filename
    if (mxIsChar(FILE_IN) != 1 || mxGetM(FILE_IN) != 1) {
        mexErrMsgTxt("adf_readdi: first arg must be a filename-string"); 
    }
    filename = mxArrayToString(FILE_IN);
    if (filename == NULL)
        mexErrMsgTxt("adf_readdi: not enough memory for the filename string.");
    // open the file
    fp = fopen(filename, "rb");
    if (!fp) {
        mexPrintf("adf_readdi: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_readdi: file not found.");
    }

    // check file format
    ftype = adfx_getFileFormat(fp);
    switch (ftype) {
    case ADF_ADFX2013_CONV :
        hx = adfx_readHeader(fp);
        break;
    case ADF_ADFX2013_UNCONV :
        fclose(fp);
        mexPrintf("adf_readdi: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_readdi: unconverted file");
        break;
    default:
        fclose(fp);
        mexPrintf("adf_readdi: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_readdi: not adfx file");
    }

    if (hx == NULL) {
        fclose(fp);
        mexPrintf("adf_readdi: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_readdi: invalid header info for adf/adfw");
    }

    if (adfx_getDiNumPorts(hx) <= 0)  {
        fclose(fp);
        mexPrintf("adf_readdi: adffile='%s'\n",filename);
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        mexErrMsgTxt("adf_readdi: no digital port recorded.");
    }

    // read data
    vals = NULL;
    switch (ftype) {
    case ADF_ADFX2013_CONV :
        portw = adfx_getDiPortWidth(hx,iport);
        if (startidx < 0) {
            status = adfx_getDiObsPeriod(hx, fp, iport, obsindex, &npts, &vals, &ditype);
        } else {
            status = adfx_getDiObsPeriodPartial(hx, fp, iport, obsindex, startidx, nsamples, &npts, &vals, &ditype);
        }
        break;
    }
    fclose(fp);  fp = NULL;

    if (status < 0) {
        if (filename != NULL)  { mxFree(filename);  filename = NULL; }
        if (status == -1)  mexErrMsgTxt("adf_readdi: port out of range.");
        if (status == -2)  mexErrMsgTxt("adf_readdi: obs period out of range.");
        if (status == -3)  mexErrMsgTxt("adf_readdi: start/nsamples out of range.");
        return;
    }

    // Create a matrix for the return argument
    dims[0] = 1;  dims[1] = npts;
    if (ditype == 'c') {
        ADF_OUT = mxCreateNumericArray(2, dims, mxUINT8_CLASS,  mxREAL);
        memcpy(mxGetData(ADF_OUT),vals,npts*sizeof(unsigned char));
    } else if (ditype == 's') {
        ADF_OUT = mxCreateNumericArray(2, dims, mxUINT16_CLASS, mxREAL);
        memcpy(mxGetData(ADF_OUT),vals,npts*sizeof(unsigned short));
    } else if (ditype == 'i') {
        ADF_OUT = mxCreateNumericArray(2, dims, mxUINT32_CLASS, mxREAL);
        memcpy(mxGetData(ADF_OUT),vals,npts*sizeof(unsigned int));
    } else if (ditype == 'l') {
        ADF_OUT = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
        memcpy(mxGetData(ADF_OUT),vals,npts*sizeof(unsigned long long));
    }
    free(vals);  vals = NULL;

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
            *mxGetPr(ADF_RATE) = (double) hx->us_per_sample /1000.;
            break;
        }
    }
    // port width
    if (nlhs > 3) {
        ADF_PORTWIDTH = mxCreateDoubleMatrix(1, 1, mxREAL);
        switch (ftype) {
        case ADF_ADFX2013_CONV :
            *mxGetPr(ADF_PORTWIDTH) = (double)adfx_getDiPortWidth(hx,iport);
            break;
        }
    }

    if (hx != NULL)  adfx_freeHeader(hx);
    if (filename != NULL)  { mxFree(filename);  filename = NULL; }

    return;
}
