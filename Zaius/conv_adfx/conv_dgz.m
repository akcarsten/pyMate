%DG_READ - MEX utility to read the event (.dgz) file.
%  DG = DG_READ(DGZFILE) will return recorded events in the dgzfile.
%
%  ESSxxx, the our task control program running on QNX will record events
%  as .evt and it is compiled and zipped as .dgz after the experiment.
%  DG_READ read such .dgz and returns event data as a structure like following.
%
%  dg = dg_read('//Win49/N/DataNeuro/A98.nm5/a98nm5_001.dgz');
%  dg =
%            e_pre: {33x1 cell}
%          e_names: [256x22 char]
%          e_types: {[1062x1 double]}
%       e_subtypes: {[1062x1 double]}
%          e_times: {[1062x1 double]}
%         e_params: {{1062x1 cell}}
%              ems: {{3x1 cell}}
%        spk_types: {[0x1 double]}
%     spk_channels: {[0x1 double]}
%        spk_times: {[0x1 double]}
%        obs_times: 0
%         filename: '//Win49/N/DataNeuro/A98.nm5/a98nm5_001.dgz'
%
%
%  TASK PARAMETERS :
%  .e_pre has control paramters set in ESSGUI.
%
%  EVENT NAMES :
%  .e_names are 256 event names.  .e_types has recorded event numbers.
%  Note that event number starts from 0 to 255 and need to add +1 for matlab
%  indexing to get event name as string.
%  For an example, event 19 is deblank(dg.e_names(19+1,:)) = 'Start Obs Period'.
%
%  EVENT TYPES, SUBTYPES, PARAMETERS AND TIMINGS
%  .e_types/e_subtypes/e_times/e_params/ems/spk_... are cell arrays of data for each
%  observation periods. Combination of e_types/e_subtypes/e_times/e_params tells what kind
%  of events(subtype, event parameter) is recorded at which timing (e_times) in mseconds.
%  For an example, in the first observation period and the first recored event,
%    dg.e_types{1}(1)    = 19;     % event type as 'Start Obs Period'
%    dg.e_subtypes{1}(1) = 0;      % subtype as 0
%    dg.e_params{1}{1}   = [0 1];  % event parameters,
%    dg_e_times{1}{1}    = 0;      % relative timing in the obsp as 0 msec
%  Note that meaning of e_params is dependent on the C code of ess system, and may differ
%  from program to program.
%
%
%  See also ADF_INFO, ADF_READ


function [] = conv_dgz(filename)

dgz_data = read_dgz(filename);
par = get_parameters(dgz_data);

meta_info = get_meta_info(dgz_data);

csv_filename = construct_output_filename(filename);
csv_filename = [csv_filename(1:end-4), '_mri_info.csv'];
writetable(meta_info, csv_filename);

if isempty(par)
    return
end

n_conditions = size(par.stim_duration, 1);

event_info = table( ...
    par.stim_times, ...
    par.stim_duration, ...
    strings(n_conditions, 1), ...
    'VariableNames', {'onset', 'duration', 'trial_type'});

if size(event_info, 1) > 1

    event_info.trial_type(1:2:end) = repmat('rest', size(event_info.trial_type(1:2:end), 1), 1);
    event_info.trial_type(2:2:end) = repmat('active', size(event_info.trial_type(2:2:end), 1), 1);

end

csv_filename = construct_output_filename(filename);
writetable(event_info, csv_filename, 'WriteVariableNames', true);

end

function [dgz_data] = read_dgz(filename)

dgz_data = dg_read(filename);

end

function [par] = get_parameters(dgz_data)

if isempty(dgz_data.e_times)
    par = [];
    return
end

par.time = dgz_data.e_times{1};
par.types = dgz_data.e_types{1};
par.names = cellstr(dgz_data.e_names);

par.start_obs = par.types == 19;        % Start Obs Period
par.end_obs = par.types == 20;          % End Obs Period
par.stim = par.types == 27;             % Stimulus
par.trial_type = par.types == 22;       % Trial type
par.stim_type = par.types == 29;        % Stimulus type

par.stim_times = par.time(par.stim) / 1000;
par.stim_duration = diff(par.stim_times);

par.stim_duration(end + 1) = (par.time(par.end_obs) / 1000) - par.stim_times(end);

par.index = unique(dgz_data.e_types{1}) + 1;
par.available = par.names(par.index);

par.mapping = cellfun(@(x, y) [y, '  ', x], par.available, ...
    cellstr(num2str(par.index - 1)), 'UniformOutput', false);
end

function [] = plot_dgz_data(par)

figure
hold all
plot(par.time, par.start_obs, 'r')
plot(par.time, par.end_obs, 'r')

plot(par.time, par.trial_type)
plot(par.time, par.stim)
plot(par.time, par.stim_type)

end

function [meta_info] = get_meta_info(dgz_data)

meta_info = table;
k = 1;
for n = 1:2:length(dgz_data.e_pre) - 1
    if ischar(dgz_data.e_pre{n}{2})

        meta_info.('parameter')(k) = {dgz_data.e_pre{n}{2}};

        if ~isempty(dgz_data.e_pre{n + 1}{2})
            meta_info.('value')(k) = {dgz_data.e_pre{n + 1}{2}};
        else
            meta_info.('value')(k) = {'empty'};
        end

        k = k + 1;

    end
end

end

function [csv_filename] = construct_output_filename(filename)
% Constructs the output HDF5 filename based on the input ADFX filename.
%
% Parameters:
%   - filename: A string containing the path to the input ADFX file.
%
% Returns:
%   - h5_filename: A string containing the path to the output HDF5 file.

current_path = split(pwd, '\');
data_path= split(filename, '\');

csv_path = fullfile(current_path{1:end-2}, data_path{end-1});

if ~exist(csv_path, 'dir')
    mkdir(csv_path)
end

csv_filename = fullfile(csv_path, [data_path{end}(1:end-3), 'csv']);
end
