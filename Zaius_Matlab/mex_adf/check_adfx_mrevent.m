function check_adfx_mrevent(adffile,gradch,varargin)
%
%
%
%


[fp,fr,fe] = fileparts(adffile);
dgzfile = fullfile(fp,[fr '.dgz']);
dgz = dg_read(dgzfile);


% read the gradient channel
hdr = adf_readHeader(adffile);
[wv, npts, sampt, adc2volts] = adf_read(adffile,0,gradch-1);


% clock correction 
t_end = dgz.e_times{1}(dgz.e_types{1} == 20);  % in msec
dgz.tfactor = (npts*sampt)/t_end;

% MR events
t_mri = dgz.e_times{1}(dgz.e_types{1} == 46);  % in msec
t_mri = t_mri * dgz.tfactor;
t_mri_pts = round(t_mri/sampt);

tmax = 1000;  % 1000ms
nmax = round(tmax/sampt);

% device number of the grad. ch
dev = hdr.dev_numbers(hdr.devices(gradch)+1);


figure('Name',[fr,fe]);
plot(wv(1:nmax),'color',[0.3 0.3 0.9]);
hold on;
grid on;
xlabel('Time (pts)');
ylabel('ADC unit');
title(strrep(sprintf('%s GradCh=%d:dev%d',fr,gradch,dev),'_','\_'));
t_mri_pts = t_mri_pts(t_mri_pts <= nmax);
tmpy = get(gca,'ylim');
for K = 1:length(t_mri_pts),
  tmpt = t_mri_pts(K);
  line([tmpt tmpt], tmpy, 'color',[0.8 0.1 0.1]);
  if K == 1,
    text(tmpt,tmpy(2)*0.9,sprintf('1st MR [%g(%gms)]',tmpt,tmpt*sampt));
  end
end
if t_mri_pts(1) > 0,
  set(gca,'xlim',[0 min([t_mri_pts(1)*5,nmax])]);
else
  set(gca,'xlim',[0 nmax]);
end

return

