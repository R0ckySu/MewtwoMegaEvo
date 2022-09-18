function ptm = super_to_ptm(G)
%SUPER_TO_PTM Summary of this function goes here
%   Converting super operator to Pauli transfer matrix
    n_qubits = log2(size(G,1))/2;
    p_basis = get_pauli_basis(n_qubits);
    p_vec = cellfun(@(x) reshape(x,[1,size(G,1)]), p_basis, 'UniformOutput',false);
    p_mat = cell2mat(p_vec');
    ptm = p_mat * G * p_mat';
end