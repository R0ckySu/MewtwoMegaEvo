function super_op = op_to_super(op)
%OP_TO_SUPER Summary of this function goes here
%   Detailed explanation goes here
    if iscell(op)
        super_op_list = [];
        super_op_mean = zeros(size(op{1}).^2);
        for i = 1:length(op)
            super_op_list = [super_op_list, {kron(op{i}', op{i})}];
            super_op_mean = super_op_mean+kron(op{i}', op{i});
        end
        super_op = super_op_mean / length(op);
    else
        super_op = kron(op', op);
    end
end