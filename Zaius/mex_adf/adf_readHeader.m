function HDR = adf_readHeader(adffile)
%ADF_READHEADER - To read header info. of a ADF file
%  PURPOSE : To read header info. of a ADF file
%
%  USAGE : h = adf_readHeader([adffile]);
%
%  VERSION :
%    1.00 12-May-2000  Yusuke MURAYAMA, MPI
%    1.01 24-Oct-2002  YM/MPI, supports adfwinfo file
%    1.10 05-Dec-2012  YM/MPI, supports adfx file
%
%  See also ADF_INFO ADF_READ
  
if ~exist('adffile','var'),
  [adffile,adfdir] = uigetfile(...
      {'*.adf;*.adfw;*.adfx', 'ADF/ADFW/ADFX Files (*.adf,*.adfw,*.adfx)'; ...
       '*.*',         'All Files (*.*)'}, ...
      'Pick an ADF/ADFW/ADFX file');
  if isequal(adffile,0) || isequal(adfdir,0), return;  end
  adffile = fullfile(adfdir,adffile);
end

if isempty(adffile),  return;  end
[adfdir,fn,fe] = fileparts(adffile);
adffile = sprintf('%s%s',fn,fe);
if isempty(adfdir),  adfdir = pwd;  end
fullpath = fullfile(adfdir,adffile);
if ~exist(fullpath,'file'),
  error('\n ERROR %s:  file not found, ''%s''.\n',mfilename,adffile);
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% initialize output
HDR = [];

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% open it
fid = fopen(fullpath,'rb');
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% read header
HDR.magic         = fread(fid,4, 'char');  HDR.magic = HDR.magic(:)';
if all(HDR.magic == [10 16 19 93]),
  fmt = 'adf';   unconv = 1;
elseif all(HDR.magic == [ 7  8 19 67]),
  fmt = 'adf';   unconv = 0;
elseif all(HDR.magic == [11 17 20 94]),
  fmt = 'adfw';  unconv = 1;
elseif all(HDR.magic == [ 8  9 20 68]),
  fmt = 'adfw';  unconv = 0;
elseif all(HDR.magic == [12 18 21 95]),
  fmt = 'adfx';  unconv = 1;
elseif all(HDR.magic == [ 9 10 21 69]),
  fmt = 'adfx';  unconv = 0;
else
  fclose(fid);
  error('\n Not ADF/ADFW/ADFX file: %s\n',fullpath);
  return;
end

switch lower(fmt)
 case {'adf' 'adfw'}
  HDR = sub_read_adfw(adffile, fid, HDR, unconv, fmt);
  if unconv == 1,
    fprintf('\n unconverted ADF/ADFW file: %s\n',fullpath);
  end
 case {'adfx'}
  HDR = sub_read_adfx(adffile, fid, HDR, unconv, fmt);
  if unconv == 1,
    fprintf('\n unconverted ADFX file: %s\n',fullpath);
  end
end

fclose(fid);

return;


% ---------------------------------------------------
function HDR = sub_read_adfw(adffile, fid, HDR, unconv, fmt)
% ---------------------------------------------------
HDR.version       = fread(fid,1, 'float');
HDR.nchannels     = fread(fid,1, 'int8');
HDR.channels      = fread(fid,16,'int8');  HDR.channels = HDR.channels(:)';
HDR.numconv       = fread(fid,1, 'int32');
HDR.prescale      = fread(fid,1, 'int32');
HDR.clock         = fread(fid,1, 'int32');
HDR.us_per_sample = fread(fid,1, 'float');
HDR.nobs          = fread(fid,1, 'int32');
if strcmpi(fmt,'adfw')
  HDR.resolution  = fread(fid,1, 'int8');
  HDR.input_range = fread(fid,1, 'int8');
  HDR.chan_gains  = fread(fid,16,'int8'); HDR.chan_gains = HDR.chan_gains(:)';
  HDR.scan_rate   = fread(fid,1, 'float');
  HDR.samp_timebase = fread(fid,1, 'int16');
  HDR.samp_interval = fread(fid,1, 'int16');
  HDR.scan_timebase = fread(fid,1, 'int16');
  HDR.scan_interval = fread(fid,1, 'int16');
  HDR.trig_logic_high = fread(fid,1, 'int16');
  HDR.trig_logic_low  = fread(fid,1, 'int16');
end

if unconv == 1,  return;  end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% read directory
fseek(fid,256,'bof');
HDR.channeloffs   = fread(fid,HDR.nchannels,'int32'); HDR.channeloffs = HDR.channeloffs(:)';
HDR.obscounts     = fread(fid,HDR.nobs,     'int32'); HDR.obscounts   = HDR.obscounts(:)';
HDR.offsets       = fread(fid,HDR.nobs,     'int32'); HDR.offsets     = HDR.offsets(:)';
  
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% read optional directory if info
if ~isempty(findstr(adffile,'adfinfo')) || ~isempty(findstr(adffile,'adfwinfo')),
  fseek(fid,128,'cof');
  HDR.startoffs     = fread(fid,HDR.nobs,     'int32'); HDR.startoffs = HDR.startoffs(:)';
  HDR.stopoffs      = fread(fid,HDR.nobs,     'int32'); HDR.stopoffs  = HDR.stopoffs(:)';
end
  
return;



% ---------------------------------------------------
function HDR = sub_read_adfx(adffile, fid, HDR, unconv, fmt)
% ---------------------------------------------------
HDR.version         = fread(fid,  1, 'float');
HDR.datestr         = fread(fid, 32, 'char')';   HDR.datestr(HDR.datestr == 254)   = [];  HDR.datestr  = deblank(char(HDR.datestr));
HDR.numdevices      = fread(fid,  1, 'int32');
HDR.dev_numbers     = fread(fid, 32, 'int8')';
HDR.adc_resolution  = fread(fid, 32, 'int8')';
HDR.us_per_sample   = fread(fid,  1, 'double');
HDR.scan_rate_hz    = fread(fid,  1, 'double');
HDR.samp_rate_hz    = fread(fid,  1, 'double');
HDR.nchannels_ai    = fread(fid,  1, 'int32');
HDR.nchannels_di    = fread(fid,  1, 'int32');
HDR.obsp_chan       = fread(fid,  1, 'int32');
HDR.numconv         = fread(fid,  1, 'int32');
HDR.nobs            = fread(fid,  1, 'int32');
HDR.offset2dir      = fread(fid,  1, 'int32');
HDR.offset2data     = fread(fid,  1, 'int32');
HDR.obsp_logic_high = fread(fid,  1, 'int16');
HDR.obsp_logic_low  = fread(fid,  1, 'int16');

fseek(fid,256,'bof');

nch = HDR.nchannels_ai + HDR.nchannels_di;
HDR.devices         = fread(fid, nch, 'int8')';
HDR.data_type       = char(fread(fid, nch, 'char')');
HDR.channels        = fread(fid, nch, 'int32')';
if HDR.version < single(1.10)
HDR.adc2volts       = fread(fid, nch, 'single')';
else
HDR.adc2volts       = fread(fid, nch, 'double')';
end

if unconv == 1,  return;  end

HDR.obscounts       = fread(fid, HDR.nobs, 'int32')';
HDR.offsets         = fread(fid, HDR.nobs, 'int64')'; 

return;
