function [wv,sampt] = plotadfraw(adffile,obs)
%
%
%
%

  
% read info file/header
infofile = sprintf('%sinfo',adffile);
head = adf_readheader(infofile);
sampt       = head.us_per_sample/1000;  % in msec
nobs        = head.nobs;
nchans      = head.nchannels;
obscounts   = head.obscounts;
startoffs   = head.startoffs;

if obs >= nobs,  obs = nobs;  end


npre = ceil(100/sampt);
npost = ceil(1000/sampt);

% read waveform
fid = fopen(adffile);
nseek = startoffs(obs) - npre*nchans;
nread = obscounts(obs) + npre + npost;
fseek(fid,256+nseek*2,'bof');
wv = fread(fid,nread*nchans,'int16');
fclose(fid);

wv = reshape(wv,nchans,length(wv)/nchans)';  % wv(:)->wv(t,chan)

t = (1:size(wv,1))*sampt;
figure;
plot(t,wv);
