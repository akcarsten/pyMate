function [w,lens,sampt] = adx_read(adxfile,obs,chan,startpts,widthpts)
%ADX_READ - reads 'ADX' formatted file.
% PURPOSE : To read 'adx' formatted data.
% USAGE :   [w,lens,sampt] = adx_read(adxfile,obs,chan,[startpts],[widthpts])
% NOTE :   'obs','chn' start from 0, not 1.
% SEEALSO : adx_info, adx_readobs, adx_obsLengths, adx_write
% VERSION : 1.00 15.08.00 YM   first release
%           1.01 31.08.00 YM   adds compatibility for ADF/ADFW
%           1.02 23.04.01 YM   adds more outputs
%           1.03 01.05.01 YM   supports partial reading
%           1.04 03.02.04 YM   improved performance on partial reading.
%
% See also ADX_INFO ADX_READOBS ADX_OBSLENGTHS ADX_WRITE  

if nargin < 3
  fprintf('usage: w = adx_read(adxfile,obs,chan,startpts,widthpts)\n');
  fprintf('args:  obs,chan >= 0, startpts>=0,withs>=1.\n');
  return;
end

if ~exist('startpts','var'), startpts = -1;  end
if ~exist('widthpts','var'), widthpts = -1;  end

w = [];
% Opens a file
fid = fopen(adxfile,'rb');
if fid == -1
  error(sprintf('adx_read : faild to open %s\n',adxfile));
end

% read header
HEADSIZE = 256;
h_magic         = fread(fid,4,'int8')';
h_version       = fread(fid,1,'float32');
h_nchannels     = fread(fid,1,'int8');

if length(find(h_magic == [20 69 9 10])) == 4,
  % ADX format
  h_us_per_sample = fread(fid,1,'float32');
  h_nobs          = fread(fid,1,'int32');
  h_datatype      = fread(fid,1,'int8');
else
  % ADF/ADFW format
  h_channels      = fread(fid,16,'int8');
  h_numconv       = fread(fid,1,'int32');
  h_prescale      = fread(fid,1,'int32');
  h_clock         = fread(fid,1,'int32');
  h_us_per_sample = fread(fid,1,'float32');
  h_nobs          = fread(fid,1,'int32');
  h_datatype      = 13;
end

% read directory
fseek(fid,HEADSIZE,'bof');

channeloffs = fread(fid,h_nchannels,'int32')';
obscounts   = fread(fid,h_nobs,'int32')';
offsets     = fread(fid,h_nobs,'int32')';
switch h_datatype
 case 10
  typestr = 'uint8';     typelen = 1;
 case 11
  typestr = 'int8';      typelen = 1;
 case 12
  typestr = 'uint16';    typelen = 2;
 case 13
  typestr = 'int16';     typelen = 2;
 case 14
  typestr = 'uint32';    typelen = 4;
 case 15
  typestr = 'int32';     typelen = 4;
 case 20
  typestr = 'float32';   typelen = 4;
 case 21
  typestr = 'double';    typelen = 8;
 otherwise
  error(sprintf('adx_read : unknown datatype %d\n',h_datatype));
end

% seek to the right position
%fprintf('%2d: chanoffs:%d offs:%d\n',chan+1,channeloffs(chan+1),offsets(obs+1));
fseek(fid,channeloffs(chan+1)+offsets(obs+1),'bof');

% move pointer and modify 'obscounts' if partial reading
if startpts >= 0,
  fseek(fid,startpts*typelen,'cof');
  obscounts(obs+1) = obscounts(obs+1) - startpts;
  if widthpts > 0 & widthpts < obscounts(obs+1),
    obscounts(obs+1) = widthpts;
  end
end

% read data
w = fread(fid,obscounts(obs+1),typestr);
% close the file
fclose(fid);

% adf compatibility
if length(find(h_magic == [7 8 19 67]))==4, w = w - 2048; end

% more outputs
if nargout > 1
  lens = length(w);
  sampt = h_us_per_sample/1000.;  % in msec
end
