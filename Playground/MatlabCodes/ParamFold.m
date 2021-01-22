function folded_param_info = ParamFold(param_data_source_dir,param_json_info_array)
%PARAMFOLD Providing information for folding up the flattened n-Dim grid 
% param vectors into N-d rect grid, and providing formatted field names for 
% extracting the HDF5 data.
% Func call e.x.
% ParamSpan('~/MewtwoMegaEvo/Playground/config_files',[{'gate1_span'},{'tau_span'},{'repeatnum1_span'}]);
% Output information:
% 
    folded_param_info = struct();
    
    spanned_param_dic = containers.Map; % Container for storing the spanned param vector
    unique_param_dic = containers.Map;  % Container for storing the unique param vector (distilled from spanned vec)
    series_of_params = length(param_json_info_array);
    param_space_dims = zeros(series_of_params,1);
    total_num_params = 0;
    param_name_key_ordered_list = cell(series_of_params,1);
    param_name_to_index_map = containers.Map;
    
    %% Load spanned n-D grid param vec and distill into its unique param vec
    for i=1:series_of_params
        file_name = '';
        if (isfield(param_json_info_array{i},'val_file'))
            param_file_name = param_json_info_array{i}.val_file;
            param_name_key_ordered_list{i} = param_file_name;
            param_vec = csvread([param_data_source_dir,filesep,param_file_name]);
            spanned_param_dic(param_file_name) = param_vec;
            unique_param_dic(replace(param_file_name,'_span','')) = unique(param_vec);
            param_space_dims(i) = numel(unique_param_dic(replace(param_file_name,'_span','')));
            if (total_num_params == 0)
                total_num_params = length(param_vec);
            elseif (total_num_params ~= length(param_vec))
                %'Length of the spanned param vecs dont agree with each other!'
            end            
        else
            param_file_name = param_json_info_array{i}.string_file;
            param_name_key_ordered_list{i} = param_file_name;
            param_vec = fileread([param_data_source_dir,filesep,param_file_name]);
            param_vec = strsplit(param_vec,'\n');
            spanned_param_dic(param_file_name) = param_vec;
            unique_param_dic(replace(param_file_name,'_span','')) = unique(param_vec);
            param_space_dims(i) = numel(unique_param_dic(replace(param_file_name,'_span','')));  
            if (total_num_params == 0)
                total_num_params = length(param_vec);
            elseif (total_num_params ~= length(param_vec))
                %'Length of the spanned param vecs dont agree with each other!'
            end           
        end

    end
    field_name_cellarray = cell(total_num_params,1);

    
    %% Complie param name string for accessing the fields of the hdf5 file
    for i=1:total_num_params
        field_name_temp = '';
        pos_temp = zeros(series_of_params,1);
        relative_location = i-1; 
        for j=1:series_of_params
            param_list_temp = spanned_param_dic(param_name_key_ordered_list{j});
            param_val_temp = '';
            if(isnumeric(param_list_temp))
                param_val_temp = num2str(param_list_temp(i),'%10.4e');
            else
                param_val_temp = param_list_temp{i};
            end
            field_name_temp = [field_name_temp,'#',param_name_key_ordered_list{j},'=',param_val_temp];
            
            if j < series_of_params
                pos_temp(j) = floor(relative_location/prod(param_space_dims(j+1:end)))+1;
                relative_location = mod(relative_location,prod(param_space_dims(j+1:end)));
            else
                pos_temp(j) = relative_location+1;
            end
        end
        field_name_cellarray{i} = field_name_temp;
        param_name_to_index_map(field_name_temp) = pos_temp;
    end
    
    folded_param_info.field_names = field_name_cellarray;
    folded_param_info.field_name_to_index_map = param_name_to_index_map;
    folded_param_info.unique_params = unique_param_dic;
    folded_param_info.param_space_dims = param_space_dims;
end

