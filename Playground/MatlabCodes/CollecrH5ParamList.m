function param_info_h5 = CollecrH5ParamList(data_path, file_name_list)
    param_info_h5 = struct();
    num_of_params_for_each_file = zeros(length(file_name_list),1);
    param_names = [];
    for f_idx = 1:length(file_name_list)
        file_name = [data_path,filesep,file_name_list{f_idx}];
%         info = h5info(file_name); 
        info = h5info(file_name, '/param_lists'); 
%         param_info_temp = [info.Attributes];
        if f_idx == 1
            param_names = {info.Datasets.Name};
        end
        for p_idx = 1:length(param_names)
            param_info_h5.(param_names{p_idx}) = h5read(file_name, ['/param_lists/',param_names{p_idx}]);
        end
        num_of_params_for_each_file(f_idx) = length(param_info_h5.(param_names{1}));
    end
    param_info_h5.param_names = param_names;
    param_info_h5.num_of_params_for_each_file = num_of_params_for_each_file;
end