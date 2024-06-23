function MewtwoAutoPlot(data)
%MewtwoAutoPlot Summary of this function goes here
%   Detailed explanation goes here
    % data = LoadMewtwoData(filepath);
    sim_config_info = data.configInfo;
    time_step_size = sim_config_info.step_size;
    observable_labels = sim_config_info.observables;
    init_labels = sim_config_info.init_states;
    subplot_dim = [length(init_labels), length(observable_labels)];
    
    param_list1 = data.collected_param_list.(data.collected_param_list.param_names{1});
    num_of_params = length(param_list);

    % 2D Plots
    if length(num_of_params)>1
        figure;
        if sim_config_info.record_all_meas
            data_time_len = cellfun('length', data.meas_all.(observable_labels{1}).(init_labels{1}));
            time_vec = linspace(0,data_time_len(1)-1,data_time_len(1))*time_step_size;
            if all(data_time_len(1)==data_time_len)
                for i=1:subplot_dim(1)
                    for j=1:subplot_dim(2)
                        subplot(subplot_dim(1), subplot_dim(2));
                        data2D = cell2mat(data.meas_all.(observable_labels{i}).(init_labels{j}));
                        
                    end
                end
            end
        else
            
        end
    end
end

