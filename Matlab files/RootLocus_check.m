BPS = tf([5/7*9.81],[1 0 0]);
Gain = 0.15;
SM = tf([224.8],[1 22.33 225.4]);
Sys = BPS*Gain*SM
rlocus(Sys);