function [wv,dx] = adxread(Ses,ExpNo,Obsp,Ch,beg,len,VERBOSE)
%ADXREAD - read an ADX file
%	wv = ADXREAD(Ses,ExpNo,Obsp,Ch,beg,len,VERBOSE), reads an ADX
%	file produced by Yusuke's facilities. The ADX files are an
%	extended version of the ADF files, and contain data with
%	smaller word-size to avoid memory problems.
%
%
% VERSION : 1.00 YM, 04.05.03

if nargin < 7,  VERBOSE = 0; end;
if nargin < 4,	Ch = 1; end;
if nargin < 3,	Obsp=1;	end;
if nargin < 2,	ExpNo = 1; end;

if nargin < 1,
  error('usage: [data,dx] = adxread(Ses,[ExpNo,Ch,Obsp]);');
end;

if isa(Ses,'char'), Ses = goto(Ses);  end
adxfile = catfilename(Ses,ExpNo,'adx');

if VERBOSE,
  [chan,obsp,sampt,obslen] = adx_info(adxfile);
  evt = expgetevt(Ses,ExpNo);
  obsp = length(evt.obs);
  NoObsp = obsp;
  NoChan = chan;
  fprintf('%s: NoChan=%d, NoObsp=%d, Sampt(sec)=%12.10f\n',...
          adxfile, chan,obsp,sampt/1000);
  for N=1:length(obslen),
    fprintf('ObsLen(%d) = %d\n', N, obslen(N));
  end;
end;

if ~(exist('beg','var') & exist('len','var')),
  wv = adx_read(adxfile,Obsp-1,Ch-1);
else
  if len <= 0,
    wv = adx_read(adxfile,Obsp-1,Ch-1);
    wv = wv(beg:end);
  else
    wv = adx_read(adxfile,Obsp-1,Ch-1,beg,len);
  end;
end;
wv = wv(:);

if nargout > 1,
  dx = sampt/1000;
end;

if ~nargout,
  AdxSig.evt.adfofs	 = 0;
  AdxSig.evt.adflen	 = obslen(Obsp);

  grp = getgrp(Ses,ExpNo);
  AdxSig.name = adxfile;
  AdxSig.session = Ses.name;
  AdxSig.grpname = grp.name;
  AdxSig.ExpNo = ExpNo;
  AdxSig.ObspNo = Obsp;
  AdxSig.ChanNo = Ch;

  AdxSig.dir.dname	= 'Cln';
  AdxSig.dir.physfile= catfilename(Ses,ExpNo,'phys');
  AdxSig.dir.evtfile	= catfilename(Ses,ExpNo,'evt');
  AdxSig.dir.adxfile = catfilename(Ses,ExpNo,'adx');
  
  AdxSig.dsp.func = 'dspsig';
  AdxSig.dsp.args = {'color';'k';'linestyle';'-';'linewidth';0.5};
  AdxSig.dsp.label = {'Time in sec'; 'ADC Units'};
  AdxSig.dat = wv;
  AdxSig.dx = sampt/1000;
  AdxSig.chan = grp.hardch;
  AdxSig.chan(grp.softch) = [];

  AdxSig.stm.ntrig	= grp.trginfo(1);
  AdxSig.stm.intertrigt = grp.trginfo(2);
  dt = (AdxSig.stm.ntrig * AdxSig.stm.intertrigt)/1000;
  
  AdxSig.stm.dt = grp.t;
  AdxSig.stm.v	= grp.v;
  
  for ObspNo = 1:NoObsp,
	AdxSig.evt.params{ObspNo} = evt.obs{ObspNo}.params;
	AdxSig.evt.mri1E{ObspNo}  = evt.obs{ObspNo}.mri1E;
	AdxSig.evt.mri{ObspNo}	 = evt.obs{ObspNo}.times.mri;
	for N=1:length(AdxSig.stm.v),
	  t{N}(:,ObspNo) = evt.obs{ObspNo}.t(:)./1000;
	end;
  end;
  
  for N=1:length(AdxSig.stm.v),
	AdxSig.stm.v{N} = [AdxSig.stm.v{N} 0];
	AdxSig.stm.dt{N} = grp.t{1} * dt;
	AdxSig.stm.dt{N}(1) = AdxSig.stm.dt{N}(1) - AdxSig.evt.adfofs;
  end;
  
  for N=1:length(AdxSig.stm.v),
	AdxSig.stm.t{N} = mean(t{N},2);
	if AdxSig.stm.t{N}(end) > AdxSig.evt.adflen,
	  AdxSig.stm.t{N}(end) = AdxSig.evt.adflen;
	else
	  AdxSig.stm.t{N}(end+1) = AdxSig.evt.adflen;
	end;
  end;

  assignin('base','AdxSig',AdxSig);
end;

