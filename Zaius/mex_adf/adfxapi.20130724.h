/*	adfxapi.h : This file represents api for adfx file
 *
 *  ver 1.00  12-Nov-2012  YM  modified from adfwapi.h.
 *  ver 1.04  03-Dec-2012  YM  use "int" instead of "long".
 *  ver 1.10  01-Mar-2013  YM  supports functions for Di.
 *  ver 1.11  22-Jul-2013  YM  use int32_t/int64_t from "stdint.h".
 *
 *
 * NOTE :            win     linux    OSX
 *                  w32 w64 x32 x64 x32 x64
 *   sizeof(short)   2   2   2   2   2   2
 *   sizeof(int)     4   4   4   4   4   4
 *   sizeof(long)    4   4   4   8   4   8  <---- be careful...
 *   sizeof(double)  8   8   8   8   8   8
 *
 *
 * HEADER : Unconverted ADFX File **************************************************
 *   Bytes 0-163: Header (Static)
 *       0:   4  char     magic[4];  (12, 18, 21, 95)
 *       4:   4  float    version;
 *       8:  32  char     datestr[32];
 *      40:   4  int32_t  numdevices;         // # of devices
 *      44:  32  char     dev_numbers[32];
 *      76:  32  char     adc_resolution[32];
 *     108:   8  double   us_per_sample;      // sampling rate of each channel
 *     116:   8  double   scan_rate_hz;       // sampling rate of each channel (Hz)
 *     124:   8  double   samp_rate_hz;       // sampling rate of ADC (Hz)
 *     132:   4  int32_t  nchannels_ai;       // # of analog inputs
 *     136:   4  int32_t  nchannels_di;       // # of digital ports (8/16bit)
 *     140:   4  int32_t  obsp_chan;          // obsp-TTL source,  channel index
 *     144:   4  int32_t  numconv;
 *     148:   4  int32_t  nobs;               // Here down only for converted adf files
 *     152:   4  int32_t  offset2dir;         // offset (bytes) to dir info
 *     156:   4  int32_t  offset2data;        // offset (bytes) to data
 *     160:   2  short    obsp_logic_high;    // OBSP-trigger threshold
 *     162:   2  short    obsp_logic_low;     // OBSP-trigger threshold
 *     164-255:  Reserved
 *   Byets 256-...: Header (Dynamic)
 *     256:      char*    devices[nchannels_ai + nchannels_di];    // device indices
 *     xxx:      char*    data_type[nchannels_ai + nchannels_di];  // 'c', 's', 'i' as char, short, int
 *     xxx:      int32_t* channels[nchannels_ai + nchannels_di];   //  +: analog-input, -:digital-input
 *     xxx:      float*   adc2volts[nchannels_ai + nchannels_di];  // scaling value from adc to volts
 *
 * DATA
 *   Each channel is aligned in every sampling.
 *   chan1,chan2,...,chan16--chan1,chan2,...,chan16--...
 *
 *
 * HEADER : Converted ADFX File ****************************************************
 *   Bytes 0-193: Header
 *       0:   4  char     magic[4];  (9, 10, 21, 69)
 *       4:   4  float    version;
 *       8:  32  char     datestr[32];
 *      40:   4  int32_t  numdevices;         // # of devices
 *      44:  32  char     dev_numbers[32];
 *      76:  32  char     adc_resolution[32];
 *     108:   8  double   us_per_sample;      // sampling rate of each channel
 *     116:   8  double   scan_rate_hz;       // sampling rate of each channel (Hz)
 *     124:   8  double   samp_rate_hz;       // sampling rate of ADC (Hz)
 *     132:   4  int32_t  nchannels_ai;       // # of analog inputs
 *     136:   4  int32_t  nchannels_di;       // # of digital ports (8/16bit)
 *     140:   4  int32_t  obsp_chan;          // obsp-TTL source,  channel index
 *     144:   4  int32_t  numconv;
 *     148:   4  int32_t  nobs;               // Here down only for converted adf files
 *     152:   4  int32_t  offset2dir;         // offset (bytes) to dir info
 *     156:   4  int32_t  offset2data;        // offset (bytes) to data
 *     160:   2  short    obsp_logic_high;    // OBSP-trigger threshold
 *     162:   2  short    obsp_logic_low;     // OBSP-trigger threshold
 *     164-255:  Reserved
 *   Byets 256-...: Header (Dynamic)
 *     256:      char*    devices   [nchannels_ai + nchannels_di];  // device indices
 *     xxx:      char*    data_type [nchannels_ai + nchannels_di];  // 'c', 's', 'i' as char, short, int
 *     xxx:      int32_t* channels  [nchannels_ai + nchannels_di];  //  +: analog-input, -:digital-input
 *     xxx:      float*   adc2volts [nchannels_ai + nchannels_di];  // scaling value from adc to volts
 *
 *     xxx:      int32_t* obscounts [nobs];                     // No. samples in each obs
 *     xxx:      int64_t* obsoffsets[nobs];                     // No. bytes to each obs
 *
 * DATA
 *   Blocks of samples: (16 bit A/D vals in shorts)
 *    Channel 0
 *     Obs 0
 *     Obs 1
 *     Obs 2
 *     ...
 *    Channel 1
 *     Obs 0
 *     Obs 1
 *     ...
*/

#ifndef _ADFXAPI_H_INCLUDED
#define _ADFXAPI_H_INCLUDED

#if defined(_MSC_VER)
# if (_MSC_VER <= 1500)
typedef __int32  int32_t;
typedef __int64  int64_t;
typedef unsigned __int32  uint32_t;
typedef unsigned __int64  uint64_t;
# elif (_MSC_VER == 1600) && defined(_INTSAFE_H_INCLUDED_)
#   pragma warning (push)
#   pragma warning (disable : 4005)
#   include <stdint.h>
#   pragma warning (pop)
# else
#   include <stdint.h>
# endif
#else
#include <stdint.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*************************************************/
/* file type */
#ifndef ADF_UNKOWN
#define ADF_UNKNOWN           (0)
#define ADF_WIN30_UNCONV      (10)
#define ADF_WIN30_CONV        (11)
#define ADF_PCI6052E_UNCONV   (20)
#define ADF_PCI6052E_CONV     (21)
#endif
#define ADF_ADFX2013_UNCONV   (30)
#define ADF_ADFX2013_CONV     (31)

// threthold for conversion (cf. 2.5V (3072 in 12bits) for QNX streamer )
#define DAQ_ANALOG_HIGH_VOLTS       (2.5)
#define DAQ_ANALOG_LOW_VOLTS        (1.0)
// threthold for streaming data
#define DAQ_ANALOG_HIGH_STRMR_VOLTS (1.8)
#define DAQ_ANALOG_LOW_STRMR_VOLTS  (0.8)



/*************************************************/
/* adfx stuff */
#pragma pack(1)
typedef struct {
    int32_t  nobs;
    int64_t *chanoffs;
    int64_t *startoffs;
    int64_t *stopoffs;
} ADFX_DIR;

typedef struct _adfx_header { 
    char     magic[4];
    float    version;
    char     datestr[32];
    int32_t  numdevices;         // # of devices
    char     dev_numbers[32];
    char     adc_resolution[32];
    double   us_per_sample;      // sampling rate of each channel
    double   scan_rate_hz;       // sampling rate of each channel (Hz)
    double   samp_rate_hz;       // sampling rate of ADC (Hz)
    int32_t  nchannels_ai;       // # of analog inputs
    int32_t  nchannels_di;       // # of digital ports (8/16bit)
    int32_t  obsp_chan;          // obsp-TTL source,  channel index
    int32_t  numconv;
    int32_t  nobs;               // Here down only for converted adf files
    int32_t  offset2dir;         // offset (bytes) to dir info
    int32_t  offset2data;        // offset (bytes) to data
    short    obsp_logic_high;    // OBSP-trigger threshold
    short    obsp_logic_low;     // OBSP-trigger threshold
    // reserved part ////////////////////////////////////////////////////
    // written to the adfx file
    char    *devices;            // device indices
    char    *data_type;          // 'c', 's', 'i' as char, short, int
    int32_t *channels;           //  !=0, +: analog-input, -:digital-input
    float   *adc2volts;          // scaling value from adc to volts
    // written to the adfx file (only in converted ones)
    int32_t *obscounts;          // No. samples in each obs
    int64_t *obsoffsets;         // No. bytes to each obs
    // created when reading
    int32_t *ai_channels;
    int32_t *di_channels;
} ADFX_HEADER;
#pragma pack()


#define ADFX_VERSION            (1.01)
#define ADFX_HEADER_STATIC_SIZE (256)
#define ADFX_DIR_STATIC_SIZE    (128)

/*************************************************/
/* prototypes */
FILE *adfx_fopen(const char *fname, const char *mode);
int adfx_fseek(FILE *fp, int64_t offset, int origin);
int adfx_fclose(FILE *fp);

ADFX_HEADER *adfx_readHeader(FILE *fp);
void adfx_freeHeader(ADFX_HEADER *h);
void adfx_initHeader(ADFX_HEADER *h, int converted);
int  adfx_channeloffs(int ch, ADFX_HEADER *h);
ADFX_DIR *adfx_newDirectory(int nchan, int nobs);
void adfx_freeDirectory(ADFX_DIR *d);
ADFX_DIR *adfx_createDirectory(FILE *fp, ADFX_HEADER *h);
ADFX_DIR *adfx_createDirectoryEx(FILE *fp, ADFX_HEADER *h, double logicHvolts, double logicLvolts);
int  adfx_convertFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, FILE *ofp);
int  adfx_getObsPeriod(ADFX_HEADER *h, FILE *fp, int channel, int obsp,
                           int *nsamps, short **vals);
int  adfx_getObsPeriodPartial(ADFX_HEADER *h, FILE *fp, int channel, int obsp,
                                  int startindx, int nread, int *nsamps, short **vals);
void adfx_printInfo(ADFX_HEADER *h, FILE *fp);
void adfx_printDirectory(ADFX_HEADER *h, ADFX_DIR *d);

// digital port
int  adfx_getDiObsPeriod(ADFX_HEADER *h, FILE *fp, int iport, int obsp,
                         int *nsamps, void **vals, char *datatype);
int  adfx_getDiObsPeriodPartial(ADFX_HEADER *h, FILE *fp, int iport, int obsp,
                                int startindx, int nread, int *nsamps, void **vals, char *datatype);



// file checker
int adfx_getFileFormat(FILE *fp);
int adfx_checkFileFormat(char *fname);
int adfx_checkMagicNumber(char *mgc);


// properties
int  adfx_getAiNumChannels(ADFX_HEADER *h);
int  adfx_getDiNumPorts(ADFX_HEADER *h);
char adfx_getAiDataType(ADFX_HEADER *h, int chan);
char adfx_getDiDataType(ADFX_HEADER *h, int iport);
int  adfx_getDiPortWidth(ADFX_HEADER *h, int iport);


// extended functions
int adfx_mkConvInfoFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, FILE *ofp);
ADFX_DIR *adfx_readDirEx(FILE *fp, ADFX_HEADER *h);
int adfx_getObsPeriodFromRawFile(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, 
                                     int channel, int obsp,int *nsamps, short **vals);
int adfx_getPartialObsPeriodFromBlocks(ADFX_HEADER *h, ADFX_DIR *d, FILE *fp, 
                                           int channel, int obsp,int *nsamps, short **vals);

#ifdef __cplusplus
}
#endif

#endif	/* end of _ADFXAPI_H_INCLUDED	*/
