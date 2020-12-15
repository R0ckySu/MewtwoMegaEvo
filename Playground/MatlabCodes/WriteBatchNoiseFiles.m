function WriteBatchNoiseFiles(path,noise_name,noise_data)
%LOADBATCHNOISEFILES Summary of this function goes here
%   Detailed explanation goes here
    parfor i =1:size(noise_data,2)
        noise_temp = noise_data(:,i);
        csvwrite([path,filesep,noise_name,num2str(i-1),'.csv'],noise_temp);
    end
end