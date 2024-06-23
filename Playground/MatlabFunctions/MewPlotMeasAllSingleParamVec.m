function MewPlotMeasAllSingleParamVec(data)
    measlabel_list = data.configInfo.observables;
    initLabel_list = data.configInfo.init_states;
    dt = data.configInfo.step_size;

    param_label = data.collected_param_list.param_names{1};
    param_vec = data.collected_param_list.(param_label);

    figure;
    set(gcf, 'Position', [100, 100, 500*length(measlabel_list), 300*length(initLabel_list)]);
    max_val = 0;
    min_val = 0;
    for j=1:length(measlabel_list)
        for i=1:length(initLabel_list)
            measLabel = measlabel_list{j};
            initLabel = initLabel_list{i};
            subplot(length(initLabel_list), length(measlabel_list), (i-1)*length(measlabel_list)+j);
        
            num_time_pts = length(data.meas_all.(measLabel).(initLabel){1});
            time_vec = (0:1:num_time_pts-1)*dt;
            [X, Y] = meshgrid(param_vec, time_vec);
            data2D = cell2mat(data.meas_all.(measLabel).(initLabel));
            max_val = max(max_val, max(max(data2D)));
            min_val = min(min_val, min(min(data2D)));
            s = surface(X, Y, data2D);
            s.EdgeColor = 'none';
            xlim([min(param_vec), max(param_vec)]);
            ylim([min(time_vec), max(time_vec)]);
            ylabel('time (s)');
            
            title([measLabel,'_{',initLabel,'}']);
            set(gca, 'FontSize', 12);
            if i ~= length(initLabel_list)
                set(gca, 'XTick', []);
            else
                xlabel(replace(param_label, '_', '-'));
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