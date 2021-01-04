function spanned_params = ParamSpan(param_data_source_dir,param_name_cell_array)
%PARAMSPAN 
% Func call e.x.
% ParamSpan('~/MewtwoMegaEvo/Playground/config_files',[{'gate1'},{'tau'},{'repeatnum1'}]);
% Assist constructing multi-dimensional parameter data file for MewtwoMegaEvo.
    params_cell_array = {};
    spanned_params = [];
    num_params = length(param_name_cell_array);
    
    for i=1:num_params
        param_vec = fileread([param_data_source_dir,filesep,param_name_cell_array{i}]);
        param_vec = strsplit(param_vec,'\n');
        params_cell_array = [params_cell_array;{param_vec}];
    end
    
    vec_sizes = zeros(num_params,1);
    for i=1:num_params
        vec_sizes(i) = length(params_cell_array{i});
    end
    total_size = prod(vec_sizes);
    
    spanned_vec_first = repmat(params_cell_array{1},[1,prod(vec_sizes(2:end))]);
    spanned_params = [spanned_params,{reshape(spanned_vec_first,[total_size,1])}];
    
    for i = 2:(num_params-1)
        spanned_vec_temp = repmat(params_cell_array{i},[prod(vec_sizes(1:i-1)),prod(vec_sizes(i+1:end))])
        spanned_vec_temp = reshape(spanned_vec_temp,[total_size,1]);
        spanned_params = [spanned_params,{spanned_vec_temp}];
    end
    
    spanned_vec_last = repmat(params_cell_array{end},[prod(vec_sizes(1:end-1)),1]);
    spanned_params = [spanned_params,{reshape(spanned_vec_last,[total_size,1])}];
    
    for i=1:num_params
        param_file_name = [param_data_source_dir,filesep,param_name_cell_array{i},'_span'];
        writecell(spanned_params{i},param_file_name);
        movefile([param_file_name,'.txt'],param_file_name);
    end
end