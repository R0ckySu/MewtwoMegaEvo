function MewPlotSingleMeasMarkerSingleParamVec(data)
    measlabel_list = data.configInfo.observables;
    initLabel_list = data.configInfo.init_states;
    dt = data.configInfo.step_size;
    param_label = data.collected_param_list.param_names{1};
    param_vec = data.collected_param_list.(param_label);

    legend_list = {};
    max_val = 0;
    min_val = 0;

    figure;
    for j=1:length(measlabel_list)
        for i=1:length(initLabel_list)
            measLabel = measlabel_list{j};
            initLabel = initLabel_list{i};        
            data1D = cell2mat(data.meas_marker.(measLabel).(initLabel));
            plot(param_vec, data1D, '.-');
            hold on;

            max_val = max(max_val, max(max(data1D)));
            min_val = min(min_val, min(min(data1D)));
            % set(gca, 'xlim', [min(time_vec), max(time_vec)]);
            legend_list{(i-1)*length(measlabel_list)+j} = [measLabel,'_{',initLabel,'}'];
        end
    end
    xlabel(replace(param_label, '_','-'));
    ylabel('<M>');
    legend(legend_list, 'Location', 'eastoutside');
    set(gca, 'ylim', [min_val, max_val]);
    set(gca, 'FontSize', 12);
    title(data.task_name);
end