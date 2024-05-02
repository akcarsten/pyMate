function adf_write(adffile,HDR,WVDATA,varargin)
%ADF_WRITE - Write ADF/ADFW file.
%  ADF_WRITE(ADFFILE,HDR,WVDATA) write HDR/wave as ADF/ADFW file.  'WVDATA' must be
%  a matrix of (time,chan) or a cell array of (time,chan).
%
%  EXAMPLE :
%    adf_write('myadf.adfw',hdr,wvdata)
%
%  VERSION :
%    0.90 03.11.08 YM  pre-release
%    0.91 24.01.17 YM  tentative support of adfx.
%
%  See also adf_readHeader adf_split

if nargin < 3,  help adf_write; return;  end

if ~iscell(WVDATA),  WVDATA = { WVDATA };  end

% OPTIONAL SETTINGS
VERBOSE = 1;
DIDATA  = {};
for N = 1:2:length(varargin),
  switch lower(varargin{N}),
   case {'verbose'}
    VERBOSE = varargin{N+1};
   case {'di' 'didata'}
    DIDATA = varargin{N+1};
    if ~iscell(DIDATA),  DIDATA = { DIDATA };  end
  end
end


[fp fr] = fileparts(adffile);

if all(HDR.magic(:)' == [ 7  8 19 67]),
  % adf
  adffile = fullfile(fp,sprintf('%s.adf',fr));
  sub_write_adfw(adffile,HDR,WVDATA,0,VERBOSE);
elseif all(HDR.magic(:)' == [ 8  9 20 68]),
  % adfw
  adffile = fullfile(fp,sprintf('%s.adfw',fr));
  sub_write_adfw(adffile,HDR,WVDATA,1,VERBOSE);
elseif all(HDR.magic(:)' == [ 9 10 21 69])
  % adfx
  adffile = fullfile(fp,sprintf('%s.adfx',fr));
  sub_write_adfx(adffile,HDR,WVDATA,DIDATA,VERBOSE);
else
  error(' ERROR %s:  unknown magic numbers.\n',mfilename);
end


return



% ============================================================
function sub_write_adfw(adffile,HDR,EVDATA,IsAdfw,VERBOSE)
% ============================================================

% ADF definition
ADF_HEADER_SIZE = 256;             % ADF header size, must be 256


% validate the header information
nchan = size(WVDATA{1},2);
nobs  = length(WVDATA);

HDR.nchannels   = nchan;
HDR.nobs        = nobs;

% create directory
HDR.channeloffs = zeros(1,nchan);    % offset for each cannel in bytes
HDR.obscounts   = zeros(1,nobs);     % number of points for each obs
HDR.offsets     = zeros(1,nobs);     % offset for each obs in bytes

tmpoffs = 0;
for iObs = 1:nobs,
  HDR.obscounts(iObs) = size(WVDATA{iObs},1);
  HDR.offsets(iObs) = tmpoffs;
  tmpoffs = tmpoffs + HDR.obscounts(iObs) * 2;  % 2 as int16
end

for iCh = 1:nchan,
  HDR.channeloffs(iCh) = tmpoffs*(iCh-1) + ADF_HEADER_SIZE + 4*nchan + 4*nobs + 4*nobs;
end

if VERBOSE
  fprintf(' %s: writing ''%s'' hdr...',mfilename,adffile);
end

% open the file
fid = fopen(adffile,'wb','ieee-le');

% write header
fwrite(fid, HDR.magic(1:4),       'int8');
fwrite(fid, HDR.version(1),       'float');
fwrite(fid, HDR.nchannels(1),     'int8');
fwrite(fid, HDR.channels(1:16),   'int8');
fwrite(fid, HDR.numconv(1),       'int32');
fwrite(fid, HDR.prescale(1),      'int32');
fwrite(fid, HDR.clock(1),         'int32');
fwrite(fid, HDR.us_per_sample(1), 'float');
fwrite(fid, HDR.nobs(1),          'int32');
if IsAdfw > 0,
  fwrite(fid, HDR.resolution(1),      'int8');
  fwrite(fid, HDR.input_range(1),     'int8');
  fwrite(fid, HDR.chan_gains(1:16),   'int8');
  fwrite(fid, HDR.scan_rate(1),       'float');
  fwrite(fid, HDR.samp_timebase(1),   'int16');
  fwrite(fid, HDR.samp_interval(1),   'int16');
  fwrite(fid, HDR.scan_timebase(1),   'int16');
  fwrite(fid, HDR.scan_interval(1),   'int16');
  fwrite(fid, HDR.trig_logic_high(1), 'int16');
  fwrite(fid, HDR.trig_logic_low(1),  'int16');
end

tmpdummy = zeros(1,ADF_HEADER_SIZE-ftell(fid),'int8');
fwrite(fid, tmpdummy, 'int8');

% write data directory
fwrite(fid, HDR.channeloffs(1:nchan), 'int32');
fwrite(fid, HDR.obscounts(1:nobs),    'int32');
fwrite(fid, HDR.offsets(1:nobs),      'int32');

if VERBOSE,
  fprintf(' data...');
end
% write data
for iCh = 1:nchan,
  for iObs = 1:nobs,
    tmpdat = int16(WVDATA{iObs}(:,iCh));
    fwrite(fid, tmpdat, 'int16');
  end
end

% close the file
fclose(fid);

if VERBOSE,
  fprintf(' done.\n');
end


return




% ============================================================
function sub_write_adfx(adffile, HDR, WVDATA, DIDATA, VERBOSE)
% ============================================================
% validate the header information
nchan_ai = size(WVDATA{1},2);
nchan_di = 0;
if ~isempty(DIDATA) && ~isempty(DIDATA{1})
  nchan_di = size(DIDATA{1},2);
end

if length(HDR.devices) ~= nchan_ai+nchan_di || ...
      length(HDR.data_type) ~= nchan_ai+nchan_di || ...
      length(HDR.adc2volts) ~= nchan_ai+nchan_di || ...
      HDR.nchannels_ai ~= nchan_ai || ...
      HDR.nchannels_di ~= nchan_di,
  error(' %s:  .devices/data_type/adc2volts/nchannels_ai/nchannels_di must be updated.\n', ...
        mfilename);
end

HDR.datestr        = HDR.datestr(1:min([32 length(HDR.datestr)]));
HDR.dev_numbers    = HDR.dev_numbers(1:min([32 length(HDR.dev_numbers)]));
HDR.adc_resolution = HDR.adc_resolution(1:min([32 length(HDR.adc_resolution)]));
HDR.nobs        = length(WVDATA);
HDR.offset2dir  = 256 + (1 + 1 + 4 + 8)*(HDR.nchannels_ai+HDR.nchannels_di);
HDR.offset2data = HDR.offset2dir + (4 + 8)*HDR.nobs;

% create directory
HDR.obscounts   = zeros(1,HDR.nobs);     % number of points for each obs
HDR.offsets     = zeros(1,HDR.nobs);     % offset for each obs in bytes

chanbytes = 0;
for K = 1:length(HDR.data_type),
  switch lower(HDR.data_type(K))
   case 'c'
    chanbytes = chanbytes + 1;
   case 's'
    chanbytes = chanbytes + 2;
   case 'i'
    chanbytes = chanbytes + 4;
   case 'l'
    chanbytes = chanbytes + 8;
  end
end

tmpoffs = 0;
for iObs = 1:HDR.nobs,
  HDR.obscounts(iObs) = size(WVDATA{iObs},1);
  if iObs > 1,
    HDR.offsets(iObs) = HDR.offsets(iObs-1) + HDR.obscounts(iObs-1)*chanbytes;
  end
  if HDR.nchannels_di > 0,
    if size(WVDATA{iObs},1) ~= size(DIDATA{iObs},1),
      error(' %s: length(ai_data) != length(di_data).\n',mfilename);
    end
  end
end

if VERBOSE
  fprintf(' %s: writing ''%s'' hdr...',mfilename,adffile);
end

% open the file
fid = fopen(adffile,'wb','ieee-le');

% write header
fwrite(fid, HDR.magic(1:4),         'int8');
fwrite(fid, HDR.version(1),         'float');
fwrite(fid, HDR.datestr,            'char')';  sub_write_dummy(fid,32-length(HDR.datestr));
fwrite(fid, HDR.numdevices(1),      'int32');
fwrite(fid, HDR.dev_numbers,        'int8')';  sub_write_dummy(fid,32-length(HDR.dev_numbers));
fwrite(fid, HDR.adc_resolution,     'int8')';  sub_write_dummy(fid,32-length(HDR.adc_resolution));
fwrite(fid, HDR.us_per_sample(1),   'double');
fwrite(fid, HDR.scan_rate_hz(1),    'double');
fwrite(fid, HDR.samp_rate_hz(1),    'double');
fwrite(fid, HDR.nchannels_ai(1),    'int32');
fwrite(fid, HDR.nchannels_di(1),    'int32');
fwrite(fid, HDR.obsp_chan(1),       'int32');
fwrite(fid, HDR.numconv(1),         'int32');
fwrite(fid, HDR.nobs(1),            'int32');
fwrite(fid, HDR.offset2dir(1),      'int32');
fwrite(fid, HDR.offset2data(1),     'int32');
fwrite(fid, HDR.obsp_logic_high(1), 'int16');
fwrite(fid, HDR.obsp_logic_low(1),  'int16');
%ftell(fid)
sub_write_dummy(fid,256-ftell(fid));
%ftell(fid)
fwrite(fid, HDR.devices,            'int8');
fwrite(fid, HDR.data_type,          'char');
fwrite(fid, HDR.channels,           'int32');
if HDR.version < single(1.10)
fwrite(fid, HDR.adc2volts,          'single');
else
fwrite(fid, HDR.adc2volts,          'double');
end
%HDR.offset2dir
%ftell(fid)
fwrite(fid, HDR.obscounts,          'int32');
fwrite(fid, HDR.offsets,            'int64');

%HDR.offset2data
%ftell(fid)

try
for iObs = 1:HDR.nobs,
  for iCh = 1:HDR.nchannels_ai,
    switch lower(HDR.data_type(iCh))
     case 'c'
      fwrite(fid,int8(WVDATA{iObs}(:,iCh)),'int8');
     case 's'
      fwrite(fid,int16(WVDATA{iObs}(:,iCh)),'int16');
     case 'i'
      fwrite(fid,int32(WVDATA{iObs}(:,iCh)),'int32');
     case 'l'
      fwrite(fid,int64(WVDATA{iObs}(:,iCh)),'int64');
    end
  end
  for iCh = 1:HDR.nchannels_di,
    switch lower(HDR.data_type(HDR.nchannels_ai+iCh))
     case 'c'
      fwrite(fid,uint8(DIDATA{iObs}(:,iCh)),'uint8');
     case 's'
      fwrite(fid,uint16(DIDATA{iObs}(:,iCh)),'uint16');
     case 'i'
      fwrite(fid,uint32(DIDATA{iObs}(:,iCh)),'uint32');
     case 'l'
      fwrite(fid,uint64(DIDATA{iObs}(:,iCh)),'uint64');
    end
  end
end
catch
  lasterr
  fclose(fid);
  keyboard
end
fclose(fid);

return

% -------------------------------------------
function sub_write_dummy(fid,nbytes)
if nbytes <= 0,  return;  end
fwrite(fid,zeros(1,nbytes,'int8'),'int8');
return

