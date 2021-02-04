function dataset = LoadMeasDataTrace(data_path)
%% LoadMeasDataTrace
% Load the whole data trace of single parameter dataset
%
%     
    %% Initialisation
    dataset = struct();
    dataset.data_path = data_path;
    dataset.meas_all_ND = struct();
    folder_name = strsplit(data_path,filesep);
    folder_name = folder_name{end};
    
    %% Load .json configs
    config_folder_name = [filesep,'config_files'];
    configFile = strcat(data_path,config_folder_name,filesep,'sim_config.json')
    dataset.configInfo = LoadJsonConfig(configFile);
    if (dataset.configInfo.record_all_meas == false) 
        error('Please enable "record_all_meas" in sim_config.json and re-run the simulation!');
    end
    dataset.task_name =  dataset.configInfo.task_name;
    
    %% Get multi-dim param info
    param_multi_dim_info = ParamFold([data_path,config_folder_name],dataset.configInfo.sweep_param_info);
    dataset.sweep_param_info = param_multi_dim_info;
    param_name_strs = param_multi_dim_info.field_names;
    
    %% Extract hdf5 Observable and initial state field name information
    observable_array = dataset.configInfo.observables;
    init_state_array = dataset.configInfo.init_states;
    
    %% Get field name lookup table for job sliced tasks    
    fieldName_jobid_LUT = GetParamName2JobIdLookupTable(data_path,'[0-9]+_Job#[0-9]+_meas_all$',dataset.configInfo.task_name);
    
    for o_idx = 1:length(observable_array)
        dataset.meas_all_ND.(observable_array{o_idx}) = struct();
        for i_idx = 1:length(init_state_array)
            % Loop over initial state symbols
            meas_all_ND_temp = cell(flip(param_multi_dim_info.param_space_dims'));
            size_of_last_data = zeros();
            can_transform_cell_array_to_mat = true;

            for j=1:length(param_name_strs)
                meas_all_data_temp = h5read([data_path,filesep,fieldName_jobid_LUT(['/',param_name_strs{j}])],['/',param_name_strs{j},'/',observable_array{o_idx},'/',init_state_array{i_idx}]);

                meas_all_ND_temp{j} = meas_all_data_temp;

                if (j==1)
                    size_of_last_data = size(meas_all_data_temp);
                elseif(isequal(size_of_last_data,size(meas_all_data_temp)) && can_transform_cell_array_to_mat)
                    size_of_last_data = size(meas_all_data_temp);
                else
                    can_transform_cell_array_to_mat = false;
                end
            end
            dataset.meas_all_ND.(observable_array{o_idx}).(init_state_array{i_idx}) = meas_all_ND_temp;
            if (can_transform_cell_array_to_mat) 
                dataset.meas_all_ND.(init_state_array{i_idx}) = cell2mat(meas_all_ND_temp);
            end
        end
    end
    
    dataset
    save([dataset.data_path,filesep,dataset.task_name,'_meas_all_organised'],'dataset');
    
end