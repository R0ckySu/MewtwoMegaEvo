function dataset = LoadMeasAllDataSet(data_path,varargin)
    %% Initialisation
    dataset = struct();
    dataset.meas_all = struct();
    dataset.meas = struct();
    folder_name = strsplit(data_path,'/');
    folder_name = folder_name{end};
    
    %% Load .json configs
    config_folder_name = [filesep,'config_files'];
    configFile = strcat(data_path,config_folder_name,filesep,'sim_config.json')
    fid = fopen(configFile);
    raw = fread(fid,inf);
    str = char(raw');
    fclose(fid);
    configInfo = jsondecode(str);
    dataset.configInfo = configInfo;
    
    %% Extract h5 field information
    observable_array = configInfo.observables;
    init_state_array = configInfo.init_states;
    
    %% Get file name patter for job sliced tasks
    meas_all_hdf5_filename_pattern = [configInfo.task_name,'[0-9]+_Job#[0-9]+_meas_all$'];
    
    result_dir = dir(data_path);
    hdf5fileNames = {result_dir.name};
    for i=1:length(hdf5fileNames)
        if (regexp(hdf5fileNames{i},meas_all_hdf5_filename_pattern))
             measall_filePath = [data_path,filesep,hdf5fileNames{i}];
             measall_info = h5info(measall_filePath);
             measall_param_fieldNames={measall_info.Groups.Name};
             for j=1:length(measall_param_fieldNames)
                figure;
                legend_label = [];
                time_vec = h5read([data_path,filesep,hdf5fileNames{i}],[measall_param_fieldNames{j},'/','time_vec']);
                for o_idx=1:length(observable_array)
                    for i_idx = 1:length(init_state_array)
                        legend_label = [legend_label,{['O:',observable_array{o_idx},',\rho:',init_state_array{i_idx}]}];
                        meas_data = h5read([data_path,filesep,hdf5fileNames{i}],[measall_param_fieldNames{j},'/',observable_array{o_idx},'/',init_state_array{i_idx}]);
                        plot(time_vec,meas_data);
                        hold on;
                    end
                end
                legend(legend_label);
                title({folder_name,['param',replace(measall_param_fieldNames{j},'/','=')]});
                set(gca,'FontSize',12);
                xlabel('evo time (s)');
                ylabel('P');
                grid on;
             end
        end
    end
end