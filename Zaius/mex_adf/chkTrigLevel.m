function varargout = chkTrigLevel(adffile)

if nargin == 0, adffile = pickfile;  end

if length(adffile) == 0, return; end

fprintf('chkTrigLevel: %s\n',adffile);

fid = fopen(adffile);

% read header
h_magic         = fread(fid,4,'int8')';
h_version       = fread(fid,1,'float32');
h_nchannels     = fread(fid,1,'int8');
h_channels      = fread(fid,16,'int8');
h_numconv       = fread(fid,1,'int32');
h_prescale      = fread(fid,1,'int32');
h_clock         = fread(fid,1,'int32');
h_us_per_sample = fread(fid,1,'float32');
h_nobs          = fread(fid,1,'int32');

nchans = h_nchannels;
sampt  = h_us_per_sample/1000.;

% read data
fseek(fid,256,'bof');
offs = round(0/sampt);
fseek(fid,nchans*offs*2,'cof');  % 2 as sizeof(int16)
wlength = round(20000/sampt);
wv = fread(fid,nchans*wlength,'int16');

fclose(fid);

% adf compatibility
if length(find(h_magic == [7 8 19 67]))==4,
  logicH = 3072; logicL = 3072;
  wv = wv - 2048;
  bitsPerVolt = 4096. /10.;
else
  logicH = 16000; logicL = 6500;
  bitsPerVolt = 65536. /10.;
end
wv = wv / bitsPerVolt;
logicH = logicH / bitsPerVolt;
logicL = logicL / bitsPerVolt;

figure;
trgsel = nchans:nchans:length(wv(:));
t=(1:length(trgsel))*sampt;
set(gca,'nextplot','add');
plot(t,wv(trgsel));
set(gca,'xlim',[min(t),max(t)],'ylim',[0 5.2]);
line(get(gca,'xlim'),[logicH logicH],'color','red');
line(get(gca,'xlim'),[logicL logicL],'color','red');

if nargout,
  varargout{1} = reshape(wv,[nchans numel(wv)/nchans]);
end
