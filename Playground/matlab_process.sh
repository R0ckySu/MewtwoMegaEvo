#!/bin/bash

# macOS matlab command
sim_result_path=$1
matlab_function_path="./MatlabFunctions/"
# Run MATLAB function with parameters
/Applications/MATLAB_R2023b.app/bin/matlab -nodisplay -nosplash -nodesktop -r "addpath(fileparts('${matlab_function_path}')); data=LoadMewtwoData('${sim_result_path}'); exit;"

# # Uncomment to enable WSL matlab command
# # Convert WSL path to Windows path
# sim_result_path=$(wslpath -a -w "$1")
# matlab_function_path=$(wslpath -a -w "./MatlabFunctions/")
# # Run MATLAB function with parameters
#  /mnt/c/Program\ Files/MATLAB/R2024a/bin/matlab.exe  -batch "addpath(fileparts('${matlab_function_path}')); data=LoadMewtwoData('${sim_result_path}'); exit;"