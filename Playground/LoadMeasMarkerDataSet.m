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
    
    original_param_vec = csvread([data_path,config_folder_name,filesep,configInfo.sweep_val_file]);
    param_name_strs = {};
    for i = 1:length(original_param_vec)
        param_name_strs = [param_name_strs,num2str(original_param_vec(i),'%10.4e')];
    end
    dataset.param_vec = original_param_vec;
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
                    dataset.meas2D.ylabel = 'configInfo.sweep_param_name';
                    figure;
                    imagesc('YData',param_vec,'CData',meas_marker2D);
                    xlabel(dataset.meas2D.xlabel);
                    ylabel(dataset.meas2D.ylabel);
                    set(gca,'FontSize',12);
                    colorbar;
                    title({folder_name,['O:',observable_array{o_idx},',\rho:',init_state_array{i_idx}]});
                    savefig(gcf,[data_path,filesep,'meas_marker']);
                    saveas(gcf,[data_path,filesep,'meas_marker'],'jpg');
                end
            end
        end
    end
    dataset
end