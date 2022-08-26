function choi_op = super_to_choi(s_op)
%SUPER_TO_CHOI Summary of this function goes here
%   Detailed explanation goes here
    shape = size(s_op);
    dim = floor(sqrt(shape(1)));
    choi_op = reshape(permute(reshape(s_op, repmat(dim,[1,4])),[4,2,3,1]), [dim^2, dim^2]);
end