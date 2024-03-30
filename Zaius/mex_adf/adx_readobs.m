function [w,lens,sampt] = adx_readobs(adxfile,obs,startpts,widthpts)
%ADX_READOBS - reads 'ADX' formatted data of all channels.
% PURPOSE : To read 'adx' formatted data of all channels
% USAGE :   [w,lens,sampt] = adx_readchans(adxfile,obs,[startpts],[widthpts])
% NOTE :   'obs' starts from 0, NOT 1.
% SEEALSO : adx_info, adx_read, adx_obsLengths, adx_write
% VERSION : 1.00  06.02.04  YM
%
% See also ADX_INFO ADX_READ ADX_WRITE

if nargin < 2,  help adx_readobs;  return;  end

if ~exist('startpts','var'), startpts = -1;  end
if ~exist('widthpts','var'), widthpts = -1;  end

[nchan,nobs,sampt] = adx_info(adxfile);

for N = nchan:-1:1,
  w(:,N) = adx_read(adxfile,obs,N-1,startpts,widthpts);
end


if nargout > 1,
  lens = size(w,1);
end
