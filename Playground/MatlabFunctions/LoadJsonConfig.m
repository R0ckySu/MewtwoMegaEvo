function configInfo = LoadJsonConfig(file_path)
%LOADJSONCONFIG Load Json config struct from .json 
%   
    fid = fopen(file_path);
    raw = fread(fid,inf);
    str = char(raw');
    fclose(fid);
    configInfo = jsondecode(str);
end

