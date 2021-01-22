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
    dataset.sweep_param_info = param_multi_dim_info;
    param_name_strs = param_multi_dim_info.field_names;

    dataset.configInfo = configInfo;
    observable_array = configInfo.observables;
    init_state_array = configInfo.init_states;
    
    meas_hdf5_filename_pattern = [configInfo.task_name,'[0-9]+_Job#[0-9]+_meas$'];
    
    result_dir = dir(data_path);
    hdf5fileNames = {result_dir.name};
    for i=1:length(hdf5fileNames)
        % Loop over job-sliced hdf5 files
        if (regexp(hdf5fileNames{i},meas_hdf5_filename_pattern))
            % Create fig for each Observable-initial state field
            measall_filePath = [data_path,filesep,hdf5fileNames{i}];
            measall_info = h5info(measall_filePath);
            measall_param_fieldNames={measall_info.Groups.Name};
%             figure;
%             set(gcf, 'Position',  [10, 10, 640*length(init_state_array), 480*length(observable_array)]);
            for o_idx=1:length(observable_array)
                % Loop over observable symbols
                dataset.measND.(observable_array{o_idx}) = struct();
                for i_idx = 1:length(init_state_array)
                    % Loop over initial state symbols
                    meas_markerND = cell(flip(param_multi_dim_info.param_space_dims'));
                    time_vecND = cell(flip(param_multi_dim_info.param_space_dims'));
                    
                    size_of_last_data =0;
                    can_transform_cell_array_to_mat = true;
                    
                    for j=1:length(param_name_strs)
                        meas_data_row = h5read([data_path,filesep,hdf5fileNames{i}],['/',param_name_strs{j},'/',observable_array{o_idx},'/',init_state_array{i_idx}]);
                        time_point_vec = h5read([data_path,filesep,hdf5fileNames{i}],['/',param_name_strs{j},'/','time_vec']);
                        
                        meas_markerND{j} = meas_data_row';
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
        end
    end
    dataset
    save([dataset.data_path,filesep,dataset.task_name,'_organised'],'dataset');
end