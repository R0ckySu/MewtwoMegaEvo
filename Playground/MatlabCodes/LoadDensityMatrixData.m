function dataset = LoadDensityMatrixData(data_path)
%% LoadDensityMatrixData

    %% Initialisation
    dataset = struct();
    dataset.data_path = data_path;
    folder_name = strsplit(data_path,filesep);
    folder_name = folder_name{end};
    
    %% Load .json configs
    config_folder_name = [filesep,'config_files'];
    configFile = strcat(data_path,config_folder_name,filesep,'sim_config.json')
    dataset.configInfo = LoadJsonConfig(configFile);
    dataset.task_name = dataset.configInfo.task_name;
    if (dataset.configInfo.record_all_density_mat == false) 
        error('Please enable "record_all_density_mat" in sim_config.json and re-run the simulation!');
    end
    
    %% Get multi-dim param info
    param_multi_dim_info = ParamFold([data_path,config_folder_name],dataset.configInfo.sweep_param_info);
    dataset.sweep_param_info = param_multi_dim_info;
    param_name_strs = param_multi_dim_info.field_names;

    %% Extract hdf5 initial state field name information
    init_state_array = dataset.configInfo.init_states;
    
    %% Get field name lookup table for job sliced tasks
    fieldName_jobid_LUT = GetParamName2JobIdLookupTable(data_path,'[0-9]+_Job#[0-9]+_dm$',dataset.configInfo.task_name);

    for i_idx = 1:length(init_state_array)
        % Loop over initial state symbols
        DM_ND = cell(flip(param_multi_dim_info.param_space_dims'));
        size_of_last_data = zeros();
        can_transform_cell_array_to_mat = true;

        for j=1:length(param_name_strs)
            dm_data_complex_struct = h5read([data_path,filesep,fieldName_jobid_LUT(['/',param_name_strs{j}])],['/',param_name_strs{j},'/',init_state_array{i_idx}]);

            DM_ND{j} = dm_data_complex_struct.real + 1i*dm_data_complex_struct.imag;

            if (j==1)
                size_of_last_data = size(dm_data_complex_struct.real);
            elseif(isequal(size_of_last_data,size(dm_data_complex_struct.real)) && can_transform_cell_array_to_mat)
                size_of_last_data = size(dm_data_complex_struct.real);
            else
                can_transform_cell_array_to_mat = false;
            end
        end
        dataset.DM_ND.(init_state_array{i_idx}) = DM_ND;
        if (can_transform_cell_array_to_mat) 
            dataset.DM_ND.(init_state_array{i_idx}) = cell2mat(DM_ND);
        end
    end
    dataset
    save([dataset.data_path,filesep,dataset.task_name,'_dm_organised'],'dataset');
end