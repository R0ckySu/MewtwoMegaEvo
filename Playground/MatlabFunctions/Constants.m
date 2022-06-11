%% Constants will be loaded to the workspace immeddiately after stem path added.
Pauli1 = struct();
Pauli2 = struct();

I = eye(2);
X = [0,1;1,0];
Y = [0,-1i;1i,0];
Z = [1,0;0,-1];

Pauli1.I = I;
Pauli1.X = X;
Pauli1.Y = Y;
Pauli1.Z = Z;

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

ST = struct();
ST.S0 = [0,1/sqrt(2),-1/sqrt(2),0]'*[0,1/sqrt(2),-1/sqrt(2),0];
ST.T0 = [0,1/sqrt(2),1/sqrt(2),0]'*[0,1/sqrt(2),1/sqrt(2),0];
ST.Tp = [1,0,0,0]'*[1,0,0,0];
ST.Tm = [0,0,0,1]'*[0,0,0,1];


STvec = struct();
STvec.S0 = [0,1/sqrt(2),-1/sqrt(2),0];
STvec.T0 = [0,1/sqrt(2),1/sqrt(2),0];
STvec.Tp = [1,0,0,0];
STvec.Tm = [0,0,0,1];

PauliSTLabels = [{'S0'}, {'T0'}, {'Tp'}, {'Tm'}];
Pauli1Labels = [{'I'},{'X'},{'Y'},{'Z'}];
Pauli2Labels = [{'II'},{'IX'},{'IY'},{'IZ'},{'XI'},{'XX'},{'XY'},{'XZ'},{'YI'},{'YX'},{'YY'},{'YZ'},{'ZI'},{'ZX'},{'ZY'},{'ZZ'}];