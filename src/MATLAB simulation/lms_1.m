function [e,w]=lms(mu,M,u,d,Delay)
%           Call:
%           [e,w]=lms(mu,M,u,d);
%
%           Input arguments:
%           mu      = step size, dim 1x1
%           M       = filter length, dim 1x1
%           u       = input signal, dim Nx1
%           d       = desired signal, dim Nx1    
%
%           Output arguments:
%           e       = estimation error, dim Nx1
%           w       = final filter coefficients, dim Mx1
Delay = 0;
%initial weights
w=zeros(M,1);

%length of input signal
N=length(u);

%make sure that u and d are colon vectors
u=u(:);
d=d(:);
e = zeros(size(d));
%LMS
% for n=max(M,Delay+1):N-Delay
%     uvec=u(n:-1:n-M+1);
%     e(n)=d(n-Delay)-w'*uvec;  
%     w=w+mu*uvec*conj(e(n));
%      mu = mu *0.9;
%     if mu <= 5e-7
%         mu = 5e-7;
%     end
% %     d(n) =  e(n);
% end
e=e(:);
