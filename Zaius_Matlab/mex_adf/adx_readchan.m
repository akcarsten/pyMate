function [w,lens,sampt] = adx_readchan(adxfile,chan,startpts,widthpts)
%ADX_READCHAN - read 'ADX' formatted data of the channels.
% PURPOSE : To read 'adx' formatted data of the channels
% USAGE :   [w,lens,sampt] = adx_readchan(adxfile,chan,[startpts],[widthpts])
% NOTE :   'chan' start from 0, not 1.
% SEEALSO : adx_info, adx_read, adx_readobs, adx_obsLengths, adx_write
% VERSION : 1.00  13-Apr-2001  YM
%           1.01  23-Apr-2001  YM  adds more outputs
%           1.02  01-May-2003  YM  supports partial reading
  
if nargin < 2
  fprintf('usage: w = adx_readchan(adxfile,obs,[startpts],[widthpts])\n');
  fprintf('args:  chan >= 0, startpts >= 0, widthpts >= 1.\n');
  return;
end

if ~exist('startpts','var'), startpts = -1;  end
if ~exist('widthpts','var'), widthpts = -1;  end

% Opens a file
fid = fopen(adxfile,'rb');
if fid == -1
  error(sprintf('adx_readchans : faild to open %s\n',adxfile));
end
w = [];
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

channeloffs = fread(fid,h_nchannels,'int32');
obscounts   = fread(fid,h_nobs,'int32');
offsets     = fread(fid,h_nobs,'int32');

switch h_datatype
 case 10
  typestr = 'uint8';
 case 11
  typestr = 'int8';
 case 12
  typestr = 'uint16';
 case 13
  typestr = 'int16';
 case 14
  typestr = 'uint32';
 case 15
  typestr = 'uint32';
 case 20
  typestr = 'float32';
 case 21
  typestr = 'double';
 otherwise
  error(sprintf('adx_readchans : unknown datatype %d\n',h_datatype));
end
% read data
w = cells(1,h_nobs);
for k=1:h_nobs
  fseek(fid,channeloffs(chan+1)+offsets(i),'bof');
  w{k} = fread(fid,obscounts(obs+1),typestr);
end
% close the file
fclose(fid);

% check partial reading
if startpts >= 0,
  startpts = startpts + 1;  % matlab indexing
  for k=1:h_nobs
	if widthpts <= 0, widthpts = length(w{k}); end
    w{k} = w{k}(startpts:(startpts+widthpts));
  end
end

% adf compatibility
if length(find(h_magic == [7 8 19 67]))==4,
  for i=1:h_nobs,  w{i} = w{i} - 2048;  end
end

% more outputs
if nargout > 1
  lens = zeros(1,h_nobs);
  for i=1:h_nobs, lens(i) = length(w{i});  end
  sampt = h_us_per_sample/1000.;  % in msec
end
