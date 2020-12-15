function noise_data = LoadBatchNoiseFiles(path,noise_name,load_num)
%LOADBATCHNOISEFILES Summary of this function goes here
%   Detailed explanation goes here
    noise_data = [];
    parfor i = 1:load_num
        noise_temp = csvread([path,filesep,noise_name,num2str(i-1),'.csv']);
        noise_data = [noise_data,noise_temp];
    end
end