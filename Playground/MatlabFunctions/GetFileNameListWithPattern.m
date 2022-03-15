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
end