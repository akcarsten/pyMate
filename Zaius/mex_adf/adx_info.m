function [nchannels,nobs,samptime,obslens,datatype] =  adx_info(adxfile)
%ADX_INFO - get information of 'ADX' formatted file.
% PURPOSE : To read information about 'adx' formatted data.
% USAGE :  [nchannels,nobs,samptime,obslens,datatype] = adx_info(adxfile)
% SEEALSO : adx_read, adx_readobs, adx_obsLengths, adx_write
% VERSION : 1.00  15-Aug-2000  YM
%           1.01  31-Aug-2000  YM, adds compatibility for ADF/ADFW
%           1.02  06-Oct-2001  YM, prints info when nargout == 0
%           1.03  01-May-2003  YM, supports 'obslens'
%
% See also ADX_READ ADX_READOBS ADX_WRITE

if nargin < 1
  fprintf('usage: [nchannels,nobs,samptime,obslens,datatype] = adx_info(adxfile)\n');
  return;
end

% Opens a file
fid = fopen(adxfile,'rb');
if fid == -1
  error(sprintf('adx_info : faild to open %s\n',adxfile));
end

ADX_HEADER_SIZE = 256;             % ADX header size, must be 256

% read header
h_magic         = fread(fid,4,'int8')';
h_version       = fread(fid,1,'float32');
h_nchannels     = fread(fid,1,'int8');
%if length(find(h_magic == [7 8 19 67]))==4 | ...
%  length(find(h_magic == [8 9 20 68]))==4
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
fseek(fid,ADX_HEADER_SIZE,'bof');
channeloffs = fread(fid,h_nchannels,'int32');
obscounts   = fread(fid,h_nobs,'int32');
offsets     = fread(fid,h_nobs,'int32');

% close it
fclose(fid);

% output
nchannels = h_nchannels;
nobs      = h_nobs;
samptime  = h_us_per_sample/1000.;
obslens   = obscounts;
switch h_datatype
 case 10
  datatype = 'uint8';
 case 11
  datatype = 'int8';
 case 12
  datatype = 'uint16';
 case 13
  datatype = 'int16';
 case 14
  datatype = 'uint32';
 case 15
  datatype = 'uint32';
 case 20
  datatype = 'float32';
 case 21
  datatype = 'double';
 otherwise
  error(sprintf('adx_info : unknown datatype %d\n',h_datatype));
end

if nargout == 0,
  if length(find(h_magic == [10, 16, 19, 93])) == 4,
    ftype = 'unconverted adf';
  elseif length(find(h_magic == [7 8 19 67])) == 4,
    ftype = 'converted adf';
  elseif length(find(h_magic == [11, 17, 20, 94])) == 4,
    ftype = 'unconverted adfw';
  elseif length(find(h_magic == [8 9 20 68])) == 4,
    ftype = 'converted adfw';
  elseif length(find(h_magic == [20 69 9 10])) == 4,
    ftype = 'adx';
  else
    ftype = 'unknown';
  end
  fprintf('adx_info: %s\n',adxfile);
  fprintf(' type:    %s,  version:  %.3f\n',ftype, h_version);
  fprintf(' nchans:  %d\n',nchannels);
  fprintf(' nobs:    %d\n',nobs);
  fprintf(' sampt:   %fms\n',samptime);
  fprintf(' data:    %s\n',datatype);
  return;
end
