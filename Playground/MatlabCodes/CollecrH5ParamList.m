function param_info_h5 = CollecrH5ParamList(data_path, file_name_list)
    param_info_h5 = struct();
    num_of_params_for_each_file = zeros(length(file_name_list),1);
    param_names = [];
    for f_idx = 1:length(file_name_list)
        file_name = [data_path,filesep,file_name_list{f_idx}];
        info = h5info(file_name); 
        param_info_temp = [info.Attributes];
        num_of_params_for_each_file(f_idx) = length(param_info_temp(f_idx).Value);
        if f_idx == 1
            for p_idx = 1:length(param_info_temp)
                param_names = [param_names, {param_info_temp(p_idx).Name}];
                param_info_h5.(param_info_temp(p_idx).Name) = param_info_temp(p_idx).Value;
            end
        else
            for p_idx = 1:length(param_info_temp)
                param_info_h5.(param_info_temp(p_idx).Name) = [param_info_h5.(param_info_temp(p_idx).Name); param_info_temp(p_idx).Value];
            end
        end
    end
    param_info_h5.param_names = param_names;
    param_info_h5.num_of_params_for_each_file = num_of_params_for_each_file;
end