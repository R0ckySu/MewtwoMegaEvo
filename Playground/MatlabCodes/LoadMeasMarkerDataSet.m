function dataset = LoadMeasMarkerDataSet(data_path)
    dataset = struct();
    dataset.meas2D = struct();
    
    folder_name = strsplit(data_path,filesep);
    folder_name = folder_name{end};
    dataset.task_name = folder_name;
    dataset.data_path = data_path;
    
    if(ispc)
        [stat,win_path_wsl_conv] = system(['wslpath -w ',data_path]);
        if (stat)
            splitted_res = strsplit(win_path_wsl_conv);
            data_path = replace(splitted_res{1},'''','');
        end
    end
    
    config_folder_name = [filesep,'config_files'];
    dataset.config_folder_name = config_folder_name;
    
    configFile = strcat(data_path,config_folder_name,filesep,'sim_config.json');
    fid = fopen(configFile);
    raw = fread(fid,inf);
    str = char(raw');
    fclose(fid);
    configInfo = jsondecode(str);
    dataset.configInfo = configInfo;
    
    param_vec_file_names = {configInfo.sweep_param_info.param_name};
    original_param_vec_struct = struct();
    num_of_params = 0;
    for i=1:length(param_vec_file_names)
        original_param_vec_struct.(param_vec_file_names{i}) = csvread([data_path,config_folder_name,filesep,param_vec_file_names{i}]);
        num_of_params = length(original_param_vec_struct.(param_vec_file_names{i}));
    end
    
    param_name_strs = {};
    for i = 1:num_of_params
        param_str_temp = '';
        for j=1:length(param_vec_file_names)
            param_str_temp = [param_str_temp,'#',param_vec_file_names{j},'=',num2str(original_param_vec_struct.(param_vec_file_names{j})(i),'%10.4e')];
        end
        param_name_strs = [param_name_strs,param_str_temp];
    end
    dataset.param_vec = original_param_vec_struct;
    observable_array = configInfo.observables;
    init_state_array = configInfo.init_states;
    
    meas_hdf5_filename_pattern = [configInfo.task_name,'[0-9]+_Job#[0-9]+_meas$'];
    
    
    result_dir = dir(data_path);
    hdf5fileNames = {result_dir.name};
    for i=1:length(hdf5fileNames)
        if (regexp(hdf5fileNames{i},meas_hdf5_filename_pattern))
            measall_filePath = [data_path,filesep,hdf5fileNames{i}];
            measall_info = h5info(measall_filePath);
            measall_param_fieldNames={measall_info.Groups.Name};
            figure;
            set(gcf, 'Position',  [10, 10, 640*length(init_state_array), 480*length(observable_array)]);
            for o_idx=1:length(observable_array)
                dataset.meas2D.(observable_array{o_idx}) = struct();
                for i_idx = 1:length(init_state_array)
                    meas_marker2D = [];
                    param_vec = [];
                    time_point2D = [];
                    for j=1:length(param_name_strs)
                        meas_data_row = h5read([data_path,filesep,hdf5fileNames{i}],['/',param_name_strs{j},'/',observable_array{o_idx},'/',init_state_array{i_idx}]);
                        time_point_vec = h5read([data_path,filesep,hdf5fileNames{i}],['/',param_name_strs{j},'/','time_vec']);
                        
                        meas_marker2D =[meas_marker2D; meas_data_row'];
                        time_point2D = [time_point2D; time_point_vec'];
                        param_vec =[param_vec, str2num(param_name_strs{j})];
                    end
                    dataset.meas2D.(observable_array{o_idx}).(init_state_array{i_idx}) = meas_marker2D;
                    dataset.meas2D.time_point2D = time_point2D;
                    dataset.meas2D.xlabel = 'n^{th} meas marker';
                    dataset.meas2D.ylabel = 'param';
                    subplot(length(observable_array),length(init_state_array),(o_idx-1)*length(init_state_array)+i_idx)
                    imagesc('YData',param_vec,'CData',meas_marker2D);
                    xlabel(dataset.meas2D.xlabel);
                    ylabel(dataset.meas2D.ylabel);
                    set(gca,'FontSize',12);
                    colorbar;
                    title({folder_name,['Observable:',observable_array{o_idx},',\rho_0:',init_state_array{i_idx}]});
                    savefig(gcf,[data_path,filesep,hdf5fileNames{i},'_2D']);
                    saveas(gcf,[data_path,filesep,hdf5fileNames{i},'_2D'],'jpg');
                end
            end
        end
    end
    dataset
end