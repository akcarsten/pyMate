function adf_split(adffile,varargin)
%ADF_SPLIT - Split ADF/ADFW to each obsp.
%  ADF_SPLIT(ADFFILE,...) splits ADF/ADFW into each observation period.
%
%  EXAMPLE :
%    adf_split('mydata.adfw')
%    adf_split('mydata.adfw','obsp',[1 2])
%
%  VERSION :
%    0.90 03.11.08 YM  pre-release
%    0.91 24.01.17 YM  supports adfx
%
%  See also adf_readHeader adf_writeHeader adf_info adf_read adf_write

if nargin == 0,  help adf_split;  return;  end


%
SELECT_OBSP = [];
SELECT_CHAN = [];
for N = 1:2:length(varargin),
  switch lower(varargin{N}),
   case {'obsp' 'obs' 'select_obsp' 'selectobsp'}
    SELECT_OBSP = varargin{N+1};
   case {'chan' 'channel' 'select_chan' 'selectchan'}
    SELECT_CHAN = varargin{N+1};
  end
end


if ~exist(adffile,'file'),
  error(' ERROR %s: ''%s'' not found.\n',mfilename,adffile);
end


HDR = adf_readHeader(adffile);
[nchan nobs sampt obslens adc2volts nport portwidth] = adf_info(adffile);
%[nchan nobs sampt obslens] = adf_info(adffile);
[fp fr fe] = fileparts(adffile);

if isempty(SELECT_OBSP),  SELECT_OBSP = 1:nobs;   end
if isempty(SELECT_CHAN),  SELECT_CHAN = 1:nchan;  end


fprintf(' %s: ''%s'' (nai+ndi=%d+%d nobs=%d) ',...
        mfilename,adffile,length(SELECT_CHAN),nport,length(SELECT_OBSP));
for N = 1:length(SELECT_OBSP),
  if mod(N,10) == 0,
    fprintf('%d',N);
  else
    fprintf('.');
  end
  obs = SELECT_OBSP(N);
  WDAT = [];  DDAT = [];
  for K = 1:length(SELECT_CHAN),
    % to be simple, assume all channels as int16(short) for adfx (in future it may not be correct...)
    tmpw = adf_read(adffile,obs-1,SELECT_CHAN(K)-1,...
                          0,obslens(obs),'int16');
    if isempty(WDAT),
      WDAT = zeros(length(tmpw),length(SELECT_CHAN),'int16');
    end
    WDAT(:,K) = tmpw(:);
  end
  for K = 1:nport,
    tmpd = adf_readdi(adffile,obs-1,K-1,0,obslens(obs));
    if isempty(DDAT),
      if max(portwidth) <= 8
        DDAT = zeros(length(tmpd),nport,'uint8');
      elseif max(portwidth) <= 16
        DDAT = zeros(length(tmpd),nport,'uint16');
      elseif max(portwidth) <= 32
        DDAT = zeros(length(tmpd),nport,'uint32');
      else
        DDAT = zeros(length(tmpd),nport,'uint64');
      end
    end
    eval(['DDAT(:,K) = ' class(DDAT) '(tmpd(:));']);
  end
  
  if all(HDR.magic(:)' == [ 9 10 21 69])
    % adfx
    tmpsel = [SELECT_CHAN(:)' (1:nport)+nchan];
    HDR.nchannels_ai= length(SELECT_CHAN);
    HDR.obsp_chan   = HDR.nchannels_ai + nport - 1;  % -1 for C/C++ indexing
    HDR.nobs        = 1;
    HDR.offset2dir  = 256 + (1 + 1 + 4 + 8)*(HDR.nchannels_ai+HDR.nchannels_di);
    HDR.offset2data = HDR.offset2dir + (4 + 8)*HDR.nobs;
    HDR.devices     = HDR.devices(tmpsel);
    HDR.data_type   = HDR.data_type(tmpsel);
    HDR.channels    = HDR.channels(tmpsel);
    HDR.adc2volts   = HDR.adc2volts(tmpsel);
    HDR.obscounts   = size(WDAT,1);
    HDR.offsets     = 0;
  else
    HDR.nobs        = 1;
    HDR.nchannels   = length(SELECT_CHAN);
    HDR.channeloffs = 256 + HDR.nchannels*4 + HDR.nobs*4 + HDR.nobs*4;
    HDR.obscounts   = size(WDAT,1);
    HDR.offsets     = 0;
  end
  newfile = fullfile(fp,sprintf('%s_obsp%03d%s',fr,obs,fe));
  adf_write(newfile,HDR,WDAT,'didata',DDAT,'verbose',0);
end
fprintf(' done.\n');


return
