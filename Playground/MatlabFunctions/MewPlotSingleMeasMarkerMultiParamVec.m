function MewPlotSingleMeasMarkerMultiParamVec(data)
% MewPlotSingleMeasMarkerMultiParamVec
% Upto 2D param supported
    
    measlabel_list = data.configInfo.observables;
    initLabel_list = data.configInfo.init_states;
    dt = data.configInfo.step_size;

    dim_first_label = '';
    dim_second_label = '';
    
    data_shape = [0,0];
    for k=1:length(data.collected_param_list.param_names)
        param_label = data.collected_param_list.param_names{k};
        param_vec = data.collected_param_list.(param_label);

        param_vec_demean = param_vec-mean(param_vec);
        f_amp = fft(param_vec_demean);
        f_amp = f_amp(2:floor(length(f_amp)/2));
        [M, M_index] = max(f_amp);
        if M_index==1
            dim_first_label = param_label;
        else
            dim_second_label = param_label;
            data_shape(2) = M_index;
            data_shape(1) = length(param_vec)/M_index;
        end
    end
    param_mesh_first = reshape(data.collected_param_list.(dim_first_label), data_shape);
    param_mesh_second = reshape(data.collected_param_list.(dim_second_label), data_shape);
    
    figure;
    set(gcf, 'Position', [100, 100, 500*length(measlabel_list), 300*length(initLabel_list)]);
    max_val = 0;
    min_val = 0;
    for j=1:length(measlabel_list)
        for i=1:length(initLabel_list)
            measLabel = measlabel_list{j};
            initLabel = initLabel_list{i};
            subplot(length(initLabel_list), length(measlabel_list), (i-1)*length(measlabel_list)+j);
        
            data2D = reshape(cell2mat(data.meas_all.(measLabel).(initLabel)), data_shape);
            max_val = max(max_val, max(max(data2D)));
            min_val = min(min_val, min(min(data2D)));
            s = surface(param_mesh_first, param_mesh_second, data2D);
            s.EdgeColor = 'none';
            xlim([min(param_mesh_first(:)), max(param_mesh_first(:))]);
            ylim([min(param_mesh_second(:)), max(param_mesh_second(:))]);
            
            ylabel(replace(dim_second_label, '_', '-'));
            
            title([measLabel,'_{',initLabel,'}']);
            set(gca, 'FontSize', 12);
            if i ~= length(initLabel_list)
                set(gca, 'XTick', []);
            else
                xlabel(replace(dim_first_label, '_', '-'));
            end
        end
        if j ~= 1
            set(gca, 'YTick', []);
        end
    end
    clim([min_val, max_val]);
    cb = colorbar;
    set(cb, 'Position', [0.93, 0.11, 0.02, 0.815]); 
    sgtitle(data.task_name);
end