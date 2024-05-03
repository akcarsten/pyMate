classdef Zaius < handle
    % Zaius - A class for converting raw data from the AGLo lab.

    properties
        sourceFilename
        targetFolder
        targetFilename
        overwrite
        header
        data
        dataType
        dgzParameter
        dgzMetaInfo
        dgzEventInfo
    end

    methods

        function obj = Zaius(filename, targetFolder, varargin)
            % Zaius - Constructor method for the Zaius class.
            %
            %   obj = Zaius(filename, targetFolder, varargin) creates a Zaius object
            %   with the specified source filename, target folder, and optional
            %   parameters.
            %
            %   Input arguments:
            %   - filename: Source filename.
            %   - targetFolder: Target folder to save converted data.
            %   - varargin (optional): Additional parameters. Specify 'dataType' to set
            %     the data type for the output HDF5 file (default: 'double').
            %
            %   Output:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.mat', 'outputfolder', 'dataType', 'double');
            %       z = Zaius('inputfile.mat', 'outputfolder');
            %

            p = inputParser;

            addRequired(p, 'filename', @ischar);
            addRequired(p, 'targetFolder', @ischar);
            addParameter(p, 'dataType', 'double', @ischar);

            parse(p, filename, targetFolder, varargin{:});

            obj.sourceFilename = p.Results.filename;
            obj.targetFolder = p.Results.targetFolder;
            obj.overwrite = true;
            obj.dataType = p.Results.dataType;

        end

        function [] = read_mat(obj)
            % read_mat - Reads data from a Matlab file that containes cleaned electrophysiological data.
            %
            %   read_mat(obj) reads data from the specified Matlab file and
            %   populates the header and data properties of the Zaius object.
            %
            %   Input arguments:
            %   - obj: Zaius object.
            %

            rawData = load(obj.sourceFilename, 'Cln');

            obj.header.sampling_rate = 1 / rawData.Cln.dx;
            obj.data = rawData.Cln.dat;

        end

        function [] = read_adfx(obj)
            % read_adfx - Reads data from an ADFX file.
            %
            %   read_adfx(obj) reads data from the specified ADFX file and populates
            %   the header and data properties of the Zaius object.
            %
            %   This method uses the adf_readHeader function to read the header
            %   information from the ADFX file and assigns it to the 'header' property
            %   of the Zaius object.
            %
            %   If the number of observations (nobs) in the header is not zero, it
            %   reads the data using adf_read function and assigns it to the 'data'
            %   property of the Zaius object. Otherwise, it assigns a zeros matrix of
            %   size (1,1) to the 'data' property.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.adfx', 'outputfolder');
            %       z.read_adfx();
            %

            obj.header = adf_readHeader(obj.sourceFilename);

            if obj.header.nobs ~= 0

                n_channels = obj.header.nchannels_ai;
                n_samples = obj.header.obscounts;

                obj.data = zeros(n_samples, n_channels);
                for i_channel = 1:n_channels
                    obj.data(:, i_channel) = adf_read(obj.sourceFilename, 0 , i_channel - 1);
                end
            else
                obj.data = zeros(1, 1);
            end

        end

        function [] = read_dgz(obj)
            % read_dgz - Reads data from a DGZ file.
            %
            %   read_dgz(obj) reads data from the specified DGZ file and assigns it
            %   to the 'data' property of the Zaius object.
            %
            %   This method utilizes the dg_read function to extract data from the
            %   DGZ file.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.dgz', 'outputfolder');
            %       z.read_dgz();
            %

            obj.data = dg_read(obj.sourceFilename);

        end

        function [] = getDgzParameters(obj)
            % get_parameters - Extracts parameters from the loaded DGZ data.
            %
            %   get_parameters(obj) extracts various parameters from the loaded DGZ
            %   data and stores them in the 'dgzParameter' property of the Zaius
            %   object.
            %
            %   This method extracts time, types, and names of events from the DGZ
            %   data and categorizes them into different types of parameters such as
            %   start observation periods, end observation periods, stimuli, trial
            %   types, and stimulus types. It also calculates the stimulus duration
            %   and maps parameter names to their corresponding indices.
            %
            %   If the loaded DGZ data does not contain event times ('e_times'), the
            %   method returns without further processing.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.dgz', 'outputfolder');
            %       z.get_parameters();
            %

            obj.dgzParameter = [];
            if isempty(obj.data.e_times)
                return
            end

            par.time = obj.data.e_times{1};
            par.types = obj.data.e_types{1};
            par.names = cellstr(obj.data.e_names);

            par.start_obs = par.types == 19;        % Start Obs Period
            par.end_obs = par.types == 20;          % End Obs Period
            par.stim = par.types == 27;             % Stimulus
            par.trial_type = par.types == 22;       % Trial type
            par.stim_type = par.types == 29;        % Stimulus type

            par.stim_times = par.time(par.stim) / 1000;
            par.stim_duration = diff(par.stim_times);

            par.stim_duration(end + 1) = (par.time(par.end_obs) / 1000) - par.stim_times(end);

            par.index = unique(obj.data.e_types{1}) + 1;
            par.available = par.names(par.index);

            par.mapping = cellfun(@(x, y) [y, '  ', x], par.available, ...
                cellstr(num2str(par.index - 1)), 'UniformOutput', false);

            obj.dgzParameter = par;

        end

        function [] = getDgzMetaInfo(obj)
            % getDgzMetaInfo - Extracts metadata information from the loaded DGZ data.
            %
            %   getDgzMetaInfo(obj) extracts metadata information from the loaded DGZ
            %   data and stores it in a table format.
            %
            %   This method iterates through the event preamble data ('e_pre') of the DGZ
            %   data and extracts parameter-value pairs. It skips over non-character
            %   values in the preamble data. If a value is empty, it is labeled as 'empty'
            %   in the metadata table.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.dgz', 'outputfolder');
            %       z.getDgzMetaInfo();
            %

            metaInfo = table;
            k = 1;
            for n = 1:2:length(obj.data.e_pre) - 1
                if ischar(obj.data.e_pre{n}{2})

                    metaInfo.('parameter')(k) = {obj.data.e_pre{n}{2}};

                    if ~isempty(obj.data.e_pre{n + 1}{2})
                        metaInfo.('value')(k) = {obj.data.e_pre{n + 1}{2}};
                    else
                        metaInfo.('value')(k) = {'empty'};
                    end

                    k = k + 1;

                end
            end

            obj.dgzMetaInfo = metaInfo;

        end

        function [] = getDgzEventInfo(obj)
            % getDgzEventInfo - Extracts event information from the loaded DGZ data.
            %
            %   getDgzEventInfo(obj) extracts event information (onset, duration, and
            %   trial type) from the loaded DGZ data and stores it in the 'dgzEventInfo'
            %   property of the Zaius object.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.dgz', 'outputfolder');
            %       z.getDgzEventInfo();
            %

            nConditions = size(obj.dgzParameter.stim_duration, 1);

            eventInfo = table( ...
                obj.dgzParameter.stim_times, ...
                obj.dgzParameter.stim_duration, ...
                strings(nConditions, 1), ...
                'VariableNames', {'onset', 'duration', 'trial_type'});

            if size(eventInfo, 1) > 1

                eventInfo.trial_type(1:2:end) = ...
                    repmat('rest', size(eventInfo.trial_type(1:2:end), 1), 1);

                eventInfo.trial_type(2:2:end) = ...
                    repmat('active', size(eventInfo.trial_type(2:2:end), 1), 1);

            end

            obj.dgzEventInfo = eventInfo;

        end

        function [] = plotDgz(obj)
            % plotDgz - Plots events from the loaded DGZ data.
            %
            %   plotDgz(obj) plots various types of events from the loaded DGZ data
            %   over time.
            %
            %   This method creates a single figure and plots different types of events
            %   (e.g., start observation periods, end observation periods, trial types,
            %   stimuli, stimulus types) over time.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.dgz', 'outputfolder');
            %       z.plotDgz();
            %

            figure
            hold all
            plot(obj.dgzParameter.time, obj.dgzParameter.start_obs, 'r')
            plot(obj.dgzParameter.time, obj.dgzParameter.end_obs, 'b')

            plot(obj.dgzParameter.time, obj.dgzParameter.trial_type)
            plot(obj.dgzParameter.time, obj.dgzParameter.stim)
            plot(obj.dgzParameter.time, obj.dgzParameter.stim_type)

            xlabel('Time');
            ylabel('Event');
            legend('Start Obs', 'End Obs', 'Trial Type', 'Stimulus', 'Stimulus Type');
            title('DGZ Data Events');

        end

        function [] = makeOutputFilename(obj, extension, varargin)
            % makeOutputFilename - Generates the target filename for saving processed data.
            %
            %   makeOutputFilename(obj, extension, varargin) generates the target filename for
            %   saving the processed data. The target filename is formed by appending
            %   the specified extension to the base filename of the source file. If the
            %   extension does not start with a '.', it is automatically added. An
            %   optional suffix can also be provided to include additional information
            %   in the filename.
            %
            %   Input arguments:
            %   - obj: Zaius object.
            %   - extension: Extension to be added to the filename (e.g., '.mat').
            %   - varargin (optional): Additional suffix to include in the filename.
            %
            %   Example:
            %       z = Zaius('inputfile.mat', 'outputfolder');
            %       z.makeOutputFilename('.h5', '_processed');
            %

            p = inputParser;
            addParameter(p, 'suffix', '', @ischar);
            parse(p, varargin{:});

            if ~startsWith(extension, '.')
                extension = ['.' extension];
            end

            [~, adfxFile] = fileparts(obj.sourceFilename);

            obj.targetFilename = fullfile(obj.targetFolder, [adfxFile, p.Results.suffix, extension]);

        end

        function [] = saveH5(obj)
            % saveH5 - Saves data to an HDF5 file along with header information.
            %
            %   saveH5(obj) saves the data stored in the 'data' property of the Zaius
            %   object to an HDF5 file specified by 'targetFilename' in the target
            %   folder specified by 'targetFolder'. It also saves the header
            %   information stored in the 'header' property of the Zaius object as
            %   attributes of the HDF5 file.
            %
            %   If the target folder does not exist, it creates the folder. If the
            %   target file already exists and the 'overwrite' flag is set to true, it
            %   overwrites the existing file. If the target file already exists and
            %   the 'overwrite' flag is set to false, it displays a warning message and
            %   returns without saving the file.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.mat', 'outputfolder');
            %       z.saveH5();
            %

            if ~exist(obj.targetFolder, 'dir')
                mkdir(obj.targetFolder)
            end

            if exist(obj.targetFilename, 'file') == 2 && obj.overwrite
                disp('Target file already exists. Overwriting the old file now.')
                delete(obj.targetFilename)
            elseif exist(obj.targetFilename, 'file') == 2 && ~obj.overwrite
                disp('Target file already exists. Delete the file or switch to overwrite mode.')
                return
            end

            h5create(obj.targetFilename, '/data', size(obj.data), 'Datatype', obj.dataType)
            h5write(obj.targetFilename,'/data', obj.data)

            parameters = fieldnames(obj.header);

            for iParameter = 1:length(parameters)

                parameterName = parameters{iParameter};
                h5writeatt(obj.targetFilename,'/', parameterName, obj.header.(parameterName));

            end
            
        end

        function [] = saveCsv(obj, csvData, varargin)
            % saveCsv - Saves data to a CSV file.
            %
            %   saveCsv(obj, csvData, varargin) saves the provided CSV data to a CSV file
            %   specified by 'targetFilename'. The user can optionally set the
            %   'WriteVariableNames' flag to determine whether variable names are written
            %   to the file.
            %
            %   Input arguments:
            %   - obj: Zaius object.
            %   - csvData: Table or matrix containing the data to be saved.
            %   - varargin (optional): Additional arguments. Set 'WriteVariableNames' to
            %     true to include variable names in the CSV file, false otherwise.
            %
            %   Example:
            %       z = Zaius('inputfile.mat', 'outputfolder');
            %       data = table(rand(5), 'VariableNames', {'Column1'});
            %       z.saveCsv(data, 'WriteVariableNames', false);
            %

            p = inputParser;
            addParameter(p, 'WriteVariableNames', true, @islogical);
            parse(p, varargin{:});

            writetable(csvData, obj.targetFilename, 'WriteVariableNames', p.Results.WriteVariableNames);

        end

        function [] = verifyResult(obj)
            % verifyResult - Verifies the result of the conversion process.
            %
            %   verifyResult(obj) reads the processed data from the target file
            %   (specified by 'targetFilename') and compares it with the data stored
            %   in the 'data' property of the Zaius object.
            %
            %   If the data from the target file does not match the 'data' property,
            %   it raises an error indicating that the conversion failed. Otherwise,
            %   it displays a message indicating that the conversion was successful.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.mat', 'outputfolder');
            %       z.verifyResult();
            %

            h5Data = h5read(obj.targetFilename, '/data');

            if ~isequal(h5Data, obj.data)
                error('Conversion failed. Input does not match output.');
            else
                disp('Conversion successful. Input matches output.');
            end
        end

        function [] = runConversion(obj)
            % runConversion - Executes the conversion process based on the file type.
            %
            %   runConversion(obj) reads the input file based on its extension and
            %   performs the appropriate conversion process. If the input file is in
            %   the DGZ format (.dgz), it reads DGZ data and extracts parameters,
            %   metadata information, and event information. It then saves the
            %   extracted information into separate CSV files.
            %
            %   For ADFX files (.adfx), it calls the 'read_adfx' method to read the
            %   data. For MAT files (.mat), it calls the 'read_mat' method to read
            %   the data. If the input file format is not supported, it displays a
            %   message indicating that a valid data file should be provided.
            %
            %   After reading the data, it generates the output filename using the
            %   'makeOutputFilename' method with the appropriate extension, saves the
            %   data to the corresponding file format, and verifies the result if
            %   applicable.
            %
            %   Input argument:
            %   - obj: Zaius object.
            %
            %   Example:
            %       z = Zaius('inputfile.adfx', 'outputfolder');
            %       z.runConversion();
            %

            if endsWith(obj.sourceFilename, '.dgz')
                obj.read_dgz;
                obj.getDgzParameters;
                obj.getDgzMetaInfo;
                obj.getDgzEventInfo;

                obj.makeOutputFilename('.csv', 'suffix', '_mri_info');
                obj.saveCsv(obj.dgzMetaInfo, 'WriteVariableNames', false);

                obj.makeOutputFilename('.csv');
                obj.saveCsv(obj.dgzEventInfo, 'WriteVariableNames', true);

                return

            elseif endsWith(obj.sourceFilename, '.adfx')
                obj.read_adfx;
            elseif endsWith(obj.sourceFilename, '.mat')
                obj.read_mat;
            else
                disp('Input file is not .adfx or .mat. Please provide a valid data file.')
            end

            obj.makeOutputFilename('.h5')
            obj.saveH5
            obj.verifyResult

        end
    end
end
