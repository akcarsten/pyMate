%
% mexadf.m : batch file to make all mex DLLs.
%
% VERSION : 1.00  02-Sep-2000  YM
%           1.01  29-Mar-2001  YM, adds cnvadfw
%           1.02  06-Dec-2012  YM, adds cnvadfx
%           1.03  01-Mar-2013  YM, adds adf_readdi
%           1.04  07-Oct-2015  YM  supports Linux-x64 (Ubuntu)

switch lower(mexext)
 case {'mexa64'}
  % adf_xxx
  %mex CFLAGS="\$CFLAGS -std=c99 -fPIC" adf_info.c          adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_info.c                   adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_read.c                   adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_readobs.c                adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_readdi.c                 adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_makeConvInfoFile.c       adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_readFileAndInfo.c        adfapi.c adfwapi.c adfxapi.c
  mex CFLAGS='-std=c99 -fPIC' adf_readFileAndInfoByTime.c  adfapi.c adfwapi.c adfxapi.c
  
  % cnvadfw/cnvadfx
  mex CFLAGS='-std=c99 -fPIC' -D_USE_IN_MATLAB cnvadfw.c   adfapi.c adfwapi.c
  mex CFLAGS='-std=c99 -fPIC' -D_USE_IN_MATLAB cnvadfx.c   adfapi.c adfwapi.c adfxapi.c
 
 otherwise
  % adf_xxx
  mex adf_info.c                   adfapi.c adfwapi.c adfxapi.c
  mex adf_read.c                   adfapi.c adfwapi.c adfxapi.c
  mex adf_readobs.c                adfapi.c adfwapi.c adfxapi.c
  mex adf_readdi.c                 adfapi.c adfwapi.c adfxapi.c
  mex adf_makeConvInfoFile.c       adfapi.c adfwapi.c adfxapi.c
  mex adf_readFileAndInfo.c        adfapi.c adfwapi.c adfxapi.c
  mex adf_readFileAndInfoByTime.c  adfapi.c adfwapi.c adfxapi.c

  % cnvadfw/cnvadfx
  mex -D_USE_IN_MATLAB cnvadfw.c   adfapi.c adfwapi.c
  mex -D_USE_IN_MATLAB cnvadfx.c   adfapi.c adfwapi.c adfxapi.c
end     
