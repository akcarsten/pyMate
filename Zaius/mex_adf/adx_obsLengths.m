function obslens = adx_obsLengths(adxfile,pts)
%ADX_OBSLENGTHS - reads obsp lenghs of 'ADX' formatted file.
% PURPOSE : To read observation lengths in 'adx' formatted data.
% USAGE :  obslens = adx_obsLengths(adxfile,[points?])
% ARGS :   obslens in msec or points
% SEEALSO : adx_info, adx_read, adx_readobs, adx_write
% VERSION : 1.00  02-Sep-2000  YM
%         : 1.01  14-Dec-2000  YM, bug fix
%         : 1.02  01-May-2000  YM, uses adx_info
%
% See also ADX_INFO ADX_READ ADX_READOBS ADX_WRITE

if nargin < 1
  fprintf('usage: obslens = adx_obsLengths(adxfile,[points?])\n');
  fprintf('note:  obslens in msec or points.\n');
  return;
end

if ~exist('pts','var'), pts = 1;  end

[nchannels,nobs,samptime,obslens,datatype] =  adx_info(adxfile);

% set output
if pts,
  obslens = obscounts;
else
  obslens = obscounts * samptime;
end
