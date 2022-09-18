function basis = get_pauli_basis(n_qubits)
    I = [1,0;0,1]/sqrt(2);
    X = [0,1;1,0]/sqrt(2);
    Y = [0,-i;i,0]/sqrt(2);
    Z = [1,0;0,-1]/sqrt(2);
    p_vec = {I;X;Y;Z};
    
    r = repmat(1:4, [n_qubits,1]);

    basis = kron_depth(p_vec, n_qubits);
    
    function m = kron_depth(op_list, dep)
        new_dep = dep-1;
        op_list_temp = [];
        for i=1:length(op_list)
            for j = 1:4
                op_temp = kron(op_list{i}, p_vec{j});
                op_list_temp = [op_list_temp,{op_temp}];
            end
        end

        if new_dep>1
            m = kron_depth(op_list_temp, new_dep);
        else
            m = op_list_temp;
        end
    end
end

