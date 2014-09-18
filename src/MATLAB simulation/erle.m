function [erle]=erle(e,d)
%           calculation of ERLE
%           Call:
%           [r]=erle(e,d)
%
%           Input arguments:
%           e       = residual echo, dim Nx1
%           d       = hybrid output signal, dim Nx1
%
%           Output arguments:
%           r       = ERLE curve in dB

%make sure that both arguments are column vectors
e=e(:);
d=d(:);

% filtering of squared signals (IIR-filter)
Pd=filter(1,[1, -0.98],d.^2); 
Pe=filter(1,[1, -0.98],e.^2);

% ERLE
erle=10*log10(Pd./Pe);
