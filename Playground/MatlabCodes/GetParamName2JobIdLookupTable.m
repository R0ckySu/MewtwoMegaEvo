function lookuptable = GetParamName2JobIdLookupTable(folder_path,sliced_file_name_pattern,task_name)
%GETPARAMNAME2JOBIDLOOKUPTABLE Construct the lookup table for mapping the
% parameter to the job-sliced hdf5 dataset
%   Detailed explanation goes here
    lookuptable = containers.Map;
    valid_file_names = GetFileNameListWithPattern(folder_path, sliced_file_name_pattern);
    
    for i =1:length(valid_file_names)
        measall_info = h5info([folder_path,filesep,valid_file_names{i}]);
        field_names = {measall_info.Groups.Name};
        for j = 1:length(field_names)
            lookuptable(field_names{j}) = valid_file_names{i};
        end
    end
end