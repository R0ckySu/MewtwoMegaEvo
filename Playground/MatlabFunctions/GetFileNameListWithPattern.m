function valid_file_names = GetFileNameListWithPattern(folder_path,task_name,sliced_file_name_pattern)
    filename_pattern = [task_name,sliced_file_name_pattern];
    result_dir = dir(folder_path);
    allfileNames = {result_dir.name};
    
    valid_file_names = {};
    for i=1:length(allfileNames)
        if (regexp(allfileNames{i},filename_pattern))
            valid_file_names =[valid_file_names,{allfileNames{i}}];
        end
    end

    file_idx_num_list = [];
    for i = 1:length(valid_file_names)
        name = strsplit(valid_file_names{i}, '#');
        file_idx_num_list = [file_idx_num_list, str2num(name{2})];
    end
    [sorted_list, sorted_idx] = sort(file_idx_num_list);
    valid_file_names = valid_file_names(sorted_idx);
end