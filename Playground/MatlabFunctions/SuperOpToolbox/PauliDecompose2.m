function K1 = PauliDecompose2(U, verbose)
    Pauli2 = struct();
    
    I = eye(2);
    X = [0,1;1,0];
    Y = [0,-1i;1i,0];
    Z = [1,0;0,-1];
    
    Pauli2.II = kron(I,I);
    Pauli2.IX = kron(I,X);
    Pauli2.IY = kron(I,Y);
    Pauli2.IZ = kron(I,Z);
    Pauli2.XI = kron(X,I);
    Pauli2.XX = kron(X,X);
    Pauli2.XY = kron(X,Y);
    Pauli2.XZ = kron(X,Z);
    Pauli2.YI = kron(Y,I);
    Pauli2.YX = kron(Y,X);
    Pauli2.YY = kron(Y,Y);
    Pauli2.YZ = kron(Y,Z);
    Pauli2.ZI = kron(Z,I);
    Pauli2.ZX = kron(Z,X);
    Pauli2.ZY = kron(Z,Y);
    Pauli2.ZZ = kron(Z,Z);

    spinor2QLabels = fieldnames(Pauli2);
    K1 = [];
    for s_i = 1:length(spinor2QLabels)
        K1 = [K1, trace(Pauli2.(spinor2QLabels{s_i})*U)/4];
    end

    if verbose
        K_table = array2table(K1);
        K_table.Properties.VariableNames = spinor2QLabels;
        disp(K_table) 
    end
end