function dataset = LoadMewtwoData(data_path)
    dataset = struct();
    dataset.meas_marker = struct();
    dataset.rho = struct();
    
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
    dataset.configInfo = configInfo;
    observable_array = configInfo.observables;
    init_state_array = configInfo.init_states;
    
%     dataset.draw = struct('field',cell(length(observable_array),length(init_state_array)));
       
    % Get field name lookup table for job sliced tasks
    all_valid_file_names = GetFileNameListWithPattern(data_path, configInfo.task_name,'[0-9]+_Job#[0-9]$');

    param_collect_struct = CollecrH5ParamList(data_path, all_valid_file_names);
    dataset.collected_param_list = param_collect_struct;
    
    % Collecting meas_marker to struct 
    for o_idx=1:length(observable_array)
        % Loop over observable symbols
        dataset.meas_marker.(observable_array{o_idx}) = struct();
        o_name = observable_array{o_idx};
        for i_idx = 1:length(init_state_array)
            i_name = init_state_array{i_idx};
            data_cell_array_collect_meas_marker = [];
            data_cell_array_collect_meas_all = [];
            num_of_params = param_collect_struct.num_of_params_for_each_file;
            for f_idx = 1:length(all_valid_file_names)
                for p_idx = 0:num_of_params(f_idx)-1
                    dataset_name = ['/meas_marker','/#',num2str(p_idx),'/',o_name,'/',i_name];
                    data_temp = h5read([data_path,filesep, all_valid_file_names{f_idx}],dataset_name);
                    data_cell_array_collect_meas_marker = [data_cell_array_collect_meas_marker, {data_temp}];

                    if configInfo.record_all_meas == 1
                        dataset_name = ['/meas_all','/#',num2str(p_idx),'/',o_name,'/',i_name];
                        data_temp = h5read([data_path,filesep, all_valid_file_names{f_idx}],dataset_name);
                        data_cell_array_collect_meas_all = [data_cell_array_collect_meas_all, {data_temp}];
                    end
                end
            end
            dataset.meas_marker.(observable_array{o_idx}).(init_state_array{i_idx}) = data_cell_array_collect_meas_marker;
            if configInfo.record_all_meas == 1
                dataset.meas_all.(observable_array{o_idx}).(init_state_array{i_idx}) = data_cell_array_collect_meas_all;
            end
        end
    end

    % Collecting density matrix to struct 
    if configInfo.record_density_mat == 1
        for i_idx = 1:length(init_state_array)
            i_name = init_state_array{i_idx};
            data_cell_array_collect_rho_marker = [];
            data_cell_array_collect_rho_all = [];
            num_of_params = param_collect_struct.num_of_params_for_each_file;
            for f_idx = 1:length(all_valid_file_names)
                for p_idx = 0:num_of_params(f_idx)-1
                    dataset_name = ['/rho_marker','/#',num2str(p_idx),'/',i_name];
                    data_temp = h5read([data_path,filesep, all_valid_file_names{f_idx}],dataset_name);
                    data_cell_array_collect_rho_marker = [data_cell_array_collect_rho_marker, {data_temp.real+1i*data_temp.imag}];
    
                    if configInfo.record_all_meas == 1
                        dataset_name = ['/rho_all','/#',num2str(p_idx),'/',i_name];
                        data_temp = h5read([data_path,filesep, all_valid_file_names{f_idx}],dataset_name);
                        data_cell_array_collect_rho_all = [data_cell_array_collect_rho_all, {data_temp}];
                    end
                end
            end
            dataset.rho_marker.(observable_array{o_idx}).(init_state_array{i_idx}) = data_cell_array_collect_rho_marker;
            if configInfo.record_all_meas == 1
                dataset.rho_all.(observable_array{o_idx}).(init_state_array{i_idx}) = data_cell_array_collect_rho_all;
            end
        end
    end

    % Collecting propagator to struct 
    if configInfo.record_propagator == 1
        data_cell_array_collect_propagator = [];
        num_of_params = param_collect_struct.num_of_params_for_each_file;
        for f_idx = 1:length(all_valid_file_names)
            for p_idx = 0:num_of_params(f_idx)-1
                dataset_name = ['/propagator','/#',num2str(p_idx)];
                data_temp = h5read([data_path,filesep, all_valid_file_names{f_idx}],dataset_name);
                data_cell_array_collect_propagator = [data_cell_array_collect_propagator, {data_temp.real+1i*data_temp.imag}];
            end
        end
        dataset.propagator = data_cell_array_collect_propagator;
    end

    dataset
    save([dataset.data_path,filesep,dataset.task_name,'_organised'],'dataset');
end