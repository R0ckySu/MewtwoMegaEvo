function MewPlotMeasAllTrace(data)
    measlabel_list = data.configInfo.observables;
    initLabel_list = data.configInfo.init_states;
    dt = data.configInfo.step_size;

    legend_list = {};
    figure;
    for j=1:length(measlabel_list)
        for i=1:length(initLabel_list)
            measLabel = measlabel_list{j};
            initLabel = initLabel_list{i};        
            num_time_pts = length(data.meas_all.(measLabel).(initLabel){1});
            time_vec = (0:1:num_time_pts-1)*dt;
            data1D = data.meas_all.(measLabel).(initLabel){1};
            plot(time_vec, data1D, '.');
            xlabel('time (s)');
            set(gca, 'xlim', [min(time_vec), max(time_vec)]);
            hold on;
            legend_list{(i-1)*length(measlabel_list)+j} = [measLabel,'_{',initLabel,'}'];
            set(gca, 'FontSize', 12);
        end
        legend(legend_list);
    end
end