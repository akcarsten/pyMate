function [] = conv_mat(filename)
% Converts ADFX file to HDF5 format and verifies the conversion.
%
% Parameters:
%   - filename: A string containing the path to the input ADFX file.

[data, header] = read_mat(filename);

h5_filename = construct_output_filename(filename);
save_as_h5(data, header, h5_filename);

verify_result(h5_filename, data);
end

function [data, header] = read_mat(filename)

data = load(filename, 'Cln');

header.sampling_rate = 1 / data.Cln.dx;
data = data.Cln.dat;

end

function [] = verify_result(h5_filename, data)
% Verifies the correctness of the conversion by comparing the saved HDF5 file with the original data.
%
% Parameters:
%   - h5_filename: A string containing the path to the saved HDF5 file.
%   - data: The original data read from the ADFX file.

h5 = h5read(h5_filename, '/data');

if ~all(all(h5 == data))
    error("conversion failed. Input does not match output")
end
end

function [h5_filename] = construct_output_filename(filename)
% Constructs the output HDF5 filename based on the input ADFX filename.
%
% Parameters:
%   - filename: A string containing the path to the input ADFX file.
%
% Returns:
%   - h5_filename: A string containing the path to the output HDF5 file.

current_path = split(pwd, '\');
data_path= split(filename, '\');

h5_path = fullfile(current_path{1:end-2}, data_path{end-1});

if ~exist(h5_path, 'dir')
    mkdir(h5_path)
end

h5_filename = fullfile(h5_path, [data_path{end}(1:end-4), '.h5'])
end

function [] = save_as_h5(data, header, filename)
% Saves data and header as an HDF5 file.
%
% Parameters:
%   - data: A matrix containing the data to be saved.
%   - header: A structure containing header information.
%   - filename: A string containing the path to the output HDF5 file.

h5create(filename, '/data', size(data), 'Datatype','double')
h5write(filename,'/data', data)

parameters = fieldnames(header);

for i_par = 1 : length(parameters)

    parameter_name = parameters{i_par};
    h5writeatt(filename,'/', parameter_name, header.(parameter_name));

end
end