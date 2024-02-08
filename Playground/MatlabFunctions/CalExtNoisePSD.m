function [PSD,omega] = CalExtNoisePSD(noise_data1,noise_data2,time_step, needplot)
%CALEXTNOISEPSD Summary of this function goes here
%   Detailed explanation goes here
    [Pxx,omega] = cpsd(noise_data1,noise_data2,[],[],[],1/time_step);
    PSD = mean(Pxx,2);
    if needplot == true
        figure;
        plot(omega(2:end),abs(real(PSD(2:end,:))));
        hold on;
        plot(omega(2:end),abs(imag(PSD(2:end,:))));
        xlabel('Freq');
        ylabel('S(\omega)');
        set(gca,'XScale','log','YScale','log');
        title('Noise PSD');
        grid on;
        legend('Re','Im');
    end
end