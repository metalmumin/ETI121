function [e,w]=nlms_1(mu,M,u,d,a,Delay,w)
%           Normalized LMS
%           Call:
%           [e,w]=nlms(mu,M,u,d,a);
%
%           Input arguments:
%           mu      = step size, dim 1x1
%           M       = filter length, dim 1x1
%           u       = input signal, dim Nx1
%           a       = constant, dim 1x1
%
%           Output arguments:
%           e       = estimation error, dim Nx1
%           w       = final filter coefficients, dim Mx1

%intial value 0
if ~exist('w','var')
    w=zeros(M,1);
else
    mu = 0.5;
end


%input signal length
N=length(u);

%make sure that u and d are colon vectors
u=u(:);
d=d(:);
Count =0;
%NLMS
for n=(M+Delay):N
    uvec=u(n-Delay:-1:n-Delay-M+1);
%     stdu = std(uvec);
    if abs(d(n)) < 0.03 %0.05
        Count = Count +1;
    else
        Count = 0;
    end
    Count = 0;
    if Count < 100 % maybe background noise detection here
        e(n)=d(n)-w'*uvec;        
        w=w+mu/(a+uvec'*uvec)*uvec*conj(e(n));
%         mu = mu * 0.95;
    else
        e(n) = d(n);
    end
    if mu < 0.5
        mu = 0.5;
    end
end

e=e(:);