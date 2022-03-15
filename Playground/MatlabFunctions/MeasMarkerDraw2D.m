function figure_handle = MeasMarkerDraw2D(data_set, vargin)
    observable_array = data_set.configInfo.observables;
    init_state_array = data_set.configInfo.init_states;
    
    num_o = length(observable_array);
    num_i = length(init_state_array);
    
    figure_handle = figure;
    figure_handle.Position = [0 0 540*num_o 400*num_i];
    
    for o_idx =1:num_o
        for i_idx = 1:num_i
            figure_handle;
            subplot(num_o,num_i,num_i*(o_idx-1)+i_idx);
            x_key = data_set.sweep_param_info.param_key_ordered{2};
            y_key = data_set.sweep_param_info.param_key_ordered{1};
            
            Xdata = data_set.sweep_param_info.axis_vec_dic(x_key);
            Ydata = data_set.sweep_param_info.axis_vec_dic(y_key);

            imagesc(Xdata,Ydata,data_set.measND.(observable_array{o_idx}).(init_state_array{i_idx}));
            view(2);
            xlabel(replace(data_set.sweep_param_info.axis_label_dic(x_key),'_',' '));
            ylabel(replace(data_set.sweep_param_info.axis_label_dic(y_key),'_',' '));
            xlim([min(Xdata), max(Xdata)]);
            colormap('turbo');
            colorbar;
            set(gca,'FontSize',12);
            title(['O=',observable_array{o_idx},',\rho=',init_state_array{i_idx}]);
        end
    end
    
    figure_handle;
    sgtitle(replace(data_set.task_name,'_',''),'FontSize',20);
    saveas(figure_handle,[data_set.data_path,filesep,data_set.task_name,'_meas_marker.png']);
    saveas(figure_handle,[data_set.data_path,filesep,data_set.task_name,'_meas_marker.fig']);
end

