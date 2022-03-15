noise_info_struct = dsp.ColoredNoise;
noise_info_struct.InverseFrequencyPower = 1;
noise_info_struct.SamplesPerFrame = 1e6;
noise_info_struct.NumChannels = 16;
noise_info_struct.BoundedOutput = true;
time_step = 1e-8;

time_vec = time_step*linspace(1,noise_info_struct.SamplesPerFrame,noise_info_struct.SamplesPerFrame);

noise_data_out = noise_info_struct();
% Square Modulation
%  noise_data_out = noise_data_out.^2;

%Noise Demean
 noise_data_out = noise_data_out - mean(noise_data_out,1).*ones(noise_info_struct.SamplesPerFrame,1);

% Amp Modulation
%  noise_data_out = noise_data_out .* (sin(2*pi*4e5*time_vec)'*ones(1,noise_info_struct.NumChannels));
% Freq Modulation
% noise_data_out = sin(2*pi*4e5*time_vec'*ones(1,noise_info_struct.NumChannels)+ noise_data_out);
% Rotating frame noise
noise_data_out = exp(1i*2*pi*1e6*time_vec'*ones(1,noise_info_struct.NumChannels).*noise_data_out);


NoiseDataName = 'PinkNon0NoiseOnRotFrame';
folderName = '/Users/rockysu/MewtwoMegaEvo/Playground';
folderName = [folderName, filesep, 'NoiseData'];
parfor i=1:noise_info_struct.NumChannels
    csvwrite([folderName,filesep,NoiseDataName,'_X#',num2str(i-1),'.csv'],real(noise_data_out(:,i)));
    csvwrite([folderName,filesep,NoiseDataName,'_Y#',num2str(i-1),'.csv'],imag(noise_data_out(:,i)));
end

[Pxx,F] = cpsd(noise_data_out,noise_data_out,[],[],[],1/time_step);
mean_PSD = mean(Pxx,2);
figure;
plot(F(2:end),Pxx(2:end,:));
xlabel('Freq');
ylabel('S(\omega)');
set(gca,'XScale','log','YScale','log');
title('Noise PSD');
grid on;