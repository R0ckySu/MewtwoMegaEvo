%% Constants will be loaded to the workspace immeddiately after stem path added.
Pauli = struct();

I = eye(2);
X = [0,1;1,0];
Y = [0,-1i;1i,0];
Z = [1,0;0,-1];

Pauli.I = I;
Pauli.X = X;
Pauli.Y = Y;
Pauli.Z = Z;

Pauli.II = kron(I,I);
Pauli.IX = kron(I,X);
Pauli.IY = kron(I,Y);
Pauli.IZ = kron(I,Z);
Pauli.XI = kron(X,I);
Pauli.XX = kron(X,X);
Pauli.XY = kron(X,Y);
Pauli.XZ = kron(X,Z);
Pauli.YI = kron(Y,I);
Pauli.YX = kron(Y,X);
Pauli.YY = kron(Y,Y);
Pauli.YZ = kron(Y,Z);
Pauli.ZI = kron(Z,I);
Pauli.ZX = kron(Z,X);
Pauli.ZY = kron(Z,Y);
Pauli.ZZ = kron(Z,Z);

ST = struct();
ST.S0 = [0,1/sqrt(2),-1/sqrt(2),0]'*[0,1/sqrt(2),-1/sqrt(2),0];
ST.T0 = [0,1/sqrt(2),1/sqrt(2),0]'*[0,1/sqrt(2),1/sqrt(2),0];
ST.Tp = [1,0,0,0]'*[1,0,0,0];
ST.Tm = [0,0,0,1]'*[0,0,0,1];
