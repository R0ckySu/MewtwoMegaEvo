function kraus_op = choi_to_kraus(choi_super_op, pauli_decomposed)
%CHOI_TO_KRAUS Summary of this function goes here
%   Detailed explanation goes here
    sup_op_size = sqrt(size(choi_super_op));

    [L, A, V] = svd(choi_super_op);
    A = diag(A);

    Kraus_op_list = cell([length(A),1]);
    for i = 1:length(A)
        Kraus_op_list{i} = reshape(sqrt(A(i))*L(:,i), sup_op_size);
    end
    kraus_op = Kraus_op_list;

    if pauli_decomposed == true
        Kraus_pauli_decomposed = cell([length(A),1]);
        for i = 1:length(A)
            if length(A) == 16
                Kraus_pauli_decomposed{i} = PauliDecompose2(Kraus_op_list{i}, false);
            elseif length(A) == 4
                Kraus_pauli_decomposed{i} = PauliDecompose(Kraus_op_list{i});
            end
        end
        kraus_op = cell2mat(Kraus_pauli_decomposed);
    end
    
end

