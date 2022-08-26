function K1 = PauliDecompose(U)
    Pauli1 = struct();    
    I = eye(2);
    X = [0,1;1,0];
    Y = [0,-1i;1i,0];
    Z = [1,0;0,-1];
    
    Pauli1.I = I;
    Pauli1.X = X;
    Pauli1.Y = Y;
    Pauli1.Z = Z;

    spinor1QLabels = fieldnames(Pauli1);
    K1 = [];
    for s_i = 1:length(spinor1QLabels)
        K1 = [K1, trace(Pauli1.(spinor1QLabels{s_i})*U)/2];
    end
end