function adx_write(wdata,fname,varargin)
%ADX_WRITE - write signals as 'ADX' format.
% PURPOSE : Writes out data with 'adx (ADF Extended)' format.
% USAGE :   adg_write(wdata,filename,...);
% ARGS :   'wdata' is a struct having members like..
%              wdata.nChan     : number of channels
%              wdata.nObs      : number of obs.
%              wdata.sampTime  : sampling time in msec
%              wdata.
%              wdata.wave      : waveform, cell(nObs,nChan)
% VARARGS : datatype, headeronly
% VERSION : 1.00 15-Aug-2000  YM
%         : 1.01 17-Apr-2002  YM  supports 'headeronly'
%         : 1.02 01-May-2003  YM  adapted to MRI analysis
% See also ADX_INFO ADX_READ ADX_READOBS ADX_OBSLENGTHS


if nargin < 2
  fprintf('usage: adx_write(data,fname,...)\n');
  return;
end

lArgin = varargin;
while length(lArgin) >= 2,
  prop = lower(lArgin{1});
  val  = lArgin{2};
  lArgin = lArgin(3:end);
  switch prop
   case {'data','datatype','dtype','type'}
    dataTypeStr = val;
   case {'headeronly', 'header'}
    headeronly = val;
  end
end
if ~exist('headeronly','var'),  headeronly = 0;          end
if ~exist('dataTypeStr','var'), dataTypeStr = 'double';  end
switch dataTypeStr
 case {'uint8'}
  dataType = 10;  dataBytes = 1;
 case {'int8','char'}
  dataType = 11;  dataBytes = 1;
 case {'uint16'}
  dataType = 12;  dataBytes = 2;
 case {'int16','short'}
  dataType = 13;  dataBytes = 2;
 case {'uint32'}
  dataType = 14;  dataBytes = 4;
 case {'int32','long'}
  dataType = 15;  dataBytes = 4;
 case {'single','float','float32'}
  dataType = 20;  dataBytes = 4;
 case {'double','float64'}
  dataType = 21;  dataBytes = 8;
end

nchan = wdata.nChan;
nobs  = wdata.nObs;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% ADX difinition
ADX_HEADER_SIZE = 256;             % ADX header size, must be 256
ADX_MAGIC       = [20 69 9 10];    % ADX magic numbers
ADX_VERSION     = 1.00;            % ADX version
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% create header
HEAD.magic      = ADX_MAGIC;       % unique magic number, see adfapi.c, adfwapi.c
HEAD.version    = ADX_VERSION;     % version
HEAD.nchannels  = nchan;           % number of channels
HEAD.us_per_sample = wdata.sampTime*1000; % in microsecond
HEAD.nobs       = nobs;            % number of observations
HEAD.datatype   = dataType;
HEAD.size       = ADX_HEADER_SIZE; % header size

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% create directory
DIRE.channeloffs = zeros(1,nchan);    % offset for each cannel in bytes
DIRE.obscounts   = zeros(1,nobs);     % number of points for each obs
DIRE.offsets     = zeros(1,nobs);     % offset for each obs in bytes
DIRE.size        = 4*(nchan + 2*nobs);% 4 as sizeof(_int32), up to 2G
n = 0;
sumn = 0;
for j=1:nobs
  n = numel(wdata.wave{j,1});  % use numel() instead of length()
  DIRE.obscounts(j) = n;
  DIRE.offsets(j) = sumn;
  sumn = sumn + n*dataBytes;
end
for i=1:nchan
  DIRE.channeloffs(i) = sumn*(i-1) + HEAD.size + DIRE.size;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% open it
fid = fopen(fname,'wb');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% write header
fwrite(fid,HEAD.magic,    'int8');
fwrite(fid,HEAD.version,  'float32');
fwrite(fid,HEAD.nchannels,'int8');
fwrite(fid,HEAD.us_per_sample,'float32');
fwrite(fid,HEAD.nobs,         'int32');
fwrite(fid,HEAD.datatype,  'int8');
% write dummy until 256 bytes
dummy=zeros(1,HEAD.size-ftell(fid));
fwrite(fid,dummy,'char');

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% write directory
fwrite(fid,DIRE.channeloffs,'int32');
fwrite(fid,DIRE.obscounts,'int32');
fwrite(fid,DIRE.offsets,'int32');

if headeronly == 1,
  fclose(fid);  return;
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% write data
for i=1:nchan
  for j=1:nobs
    c = fwrite(fid,wdata.wave{j,i},dataTypeStr);
  end
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% close it
fclose(fid);
