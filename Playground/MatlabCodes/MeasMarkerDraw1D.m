function figure_handle = MeasMarkerDraw1D(data_set, vargin)
    observable_array = data_set.configInfo.observables;
    init_state_array = data_set.configInfo.init_states;
    
    num_o = length(observable_array);
    num_i = length(init_state_array);
    
    figure_handle = figure;
%     figure_handle.Position = [0 0 540*num_i 400*num_o];
    legend_list = [];
    for o_idx =1:num_o
        for i_idx = 1:num_i
            figure_handle;
            y_key = data_set.sweep_param_info.param_key_ordered{1};
            
            Ydata = data_set.sweep_param_info.axis_vec_dic(y_key);

            plot(Ydata,data_set.measND.(observable_array{o_idx}).(init_state_array{i_idx}));
            hold on;
            
            ylabel('Prob');
            xlabel(replace(data_set.sweep_param_info.axis_label_dic(y_key),'_',' '));
            set(gca,'FontSize',12);
            label = ['O=',observable_array{o_idx},',\rho=',init_state_array{i_idx}];
            legend_list = [legend_list,{label}];
        end
    end
    
    figure_handle;
    grid on;
    legend(legend_list);
    title(replace(data_set.task_name,'_',''),'FontSize',20);
    saveas(figure_handle,[data_set.data_path,filesep,data_set.task_name,'_meas_marker.png']);
    saveas(figure_handle,[data_set.data_path,filesep,data_set.task_name,'_meas_marker.fig']);
end