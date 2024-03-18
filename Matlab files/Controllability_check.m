Ab = [0 1 0 0; 0 0 -5/7*9.81 0; 0 0 0 1; 0 0 -225.4 -22.33];
Bb = [0; 0; 0; 225.4];
Cb = [1 0 0 0];
Db = [0];
sys = ss(Ab,Bb,Cb,Db);
[b,a] = ss2tf(Ab,Bb,Cb,Db);
sstf = tf(b,a)*0.15;
Co = ctrb(sys)
rank(Co)
Ob = obsv(sys)
rank(Ob)
%--------