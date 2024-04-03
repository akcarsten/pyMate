function edtTrigLevel(adffile)

if nargin == 0, adffile = pickfile;  end

if length(adffile) == 0, return; end

fprintf('edtTrigLevel: %s\n',adffile);

fid = fopen(adffile,'a+');

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
npts = floor(10./sampt);  % 10msec
data = fread(fid,npts*nchans,'int16');
% set trigger level to low
for i=1:npts
   data(i+h_nchannels-1) = 0;
end

% write data
fseek(fid,256,'bof');  % skip header region
fwrite(fid,data,'int16');

% close fid
fclose(fid);
