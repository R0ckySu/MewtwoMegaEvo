function dataset = LoadMeasMarkerData(data_path)
    dataset = struct();
    dataset.measND = struct();
    
    folder_name = strsplit(data_path,filesep);
    folder_name = folder_name{end};
    dataset.task_name = folder_name;
    dataset.data_path = data_path;
    
    config_folder_name = [filesep,'config_files'];
    dataset.config_folder_name = config_folder_name;
    
    configFile = strcat(data_path,config_folder_name,filesep,'sim_config.json');
    fid = fopen(configFile);
    raw = fread(fid,inf);
    str = char(raw');
    fclose(fid);
    configInfo = jsondecode(str);
    
    param_multi_dim_info = ParamFold([data_path,config_folder_name],configInfo.sweep_param_info);
    param_name_strs = param_multi_dim_info.field_names;

    dataset.configInfo = configInfo;
    observable_array = configInfo.observables;
    init_state_array = configInfo.init_states;
    
    dataset.draw = struct('field',cell(length(observable_array),length(init_state_array)));
    
    num_meas_markers = 1;
    
    
    %% Get field name lookup table for job sliced tasks
    fieldName_jobid_LUT = GetParamName2JobIdLookupTable(data_path,'[0-9]+_Job#[0-9]+_meas$',dataset.configInfo.task_name);
    
    for o_idx=1:length(observable_array)
        % Loop over observable symbols
        dataset.measND.(observable_array{o_idx}) = struct();
        for i_idx = 1:length(init_state_array)
            % Loop over initial state symbols
            meas_markerND = cell(param_multi_dim_info.param_space_dims');
            time_vecND = cell(param_multi_dim_info.param_space_dims');
            
            size_of_last_data =0;
            can_transform_cell_array_to_mat = true;

            for j=1:length(param_name_strs)
                meas_data_row = h5read([data_path,filesep,fieldName_jobid_LUT(['/',param_name_strs{j}])],['/',param_name_strs{j},'/',observable_array{o_idx},'/',init_state_array{i_idx}]);
                time_point_vec = h5read([data_path,filesep,fieldName_jobid_LUT(['/',param_name_strs{j}])],['/',param_name_strs{j},'/','time_vec']);

                meas_markerND{j} = meas_data_row';
                num_meas_markers = length(meas_data_row);

                time_vecND{j} = time_point_vec';

                if (j==1)
                    size_of_last_data = length(meas_data_row);
                elseif((size_of_last_data == length(meas_data_row)) && can_transform_cell_array_to_mat)
                    size_of_last_data = length(meas_data_row);
                else
                    can_transform_cell_array_to_mat = false;
                end
            end
            dataset.measND.(observable_array{o_idx}).(init_state_array{i_idx}) = meas_markerND;
            dataset.measND.time_pointND = time_vecND;
            if (can_transform_cell_array_to_mat) 
                dataset.measND.(observable_array{o_idx}).(init_state_array{i_idx}) = cell2mat(meas_markerND);
                dataset.measND.time_pointND = cell2mat(time_vecND);
            end
            
        end
    end
    
    if (num_meas_markers>1)
        param_multi_dim_info.axis_vec_dic('meas_marker') = linspace(1,num_meas_markers,num_meas_markers);
        param_multi_dim_info.param_space_dims = [param_multi_dim_info.param_space_dims;length(meas_markerND)];
        param_multi_dim_info.axis_label_dic('meas_marker') = 'Sequence.ith meas marker';
        param_multi_dim_info.param_key_ordered{length(param_multi_dim_info.param_space_dims)} = 'meas_marker';
    end
    
    dataset.sweep_param_info = param_multi_dim_info;

    dataset
    save([dataset.data_path,filesep,dataset.task_name,'_organised'],'dataset');
end