A = [0 1 0 0; 0 0 -5/7*9.81 0; 0 0 0 1; 0 0 -225.4 -22.33];
B = [0; 0; 0; 70.1];
C = [(1*800/0.34)+300 0 0 0];
D = [0];
BPS = ss(A,B,C,D);
DBPS = c2d(BPS,0.08)
% Checking for controllability 
Co = ctrb(BPS);
rank(Co);  
Ob = obsv(BPS);
rank(Ob);

% selecting Q & R parameters and estimating the control gains
Q =[50 0 0 0; 0 1 0 0; 0 0 100 0; 0 0 0 1];
R = 0.001;
Kr = dlqr(DBPS.A,DBPS.B,Q,R);
w= eye(1);
v = 0.01*eye(4);
Estss = ss(DBPS.A, DBPS.B, DBPS.C, DBPS.D);
[Kess,Ke] = kalman(Estss,w,0.01);