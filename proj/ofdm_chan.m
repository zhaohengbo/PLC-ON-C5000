function rec = ofdm_chan( tx, snr, chanTapWeights )
% ofdm_chan     simulates a power line channel
% 
%  Copyright (C) 2002 Texas Instruments Incorporated
%  Texas Instruments Proprietary Information
%  Use subject to terms and conditions of TI Software License Agreement
% 
%  Revision History:
%  06/01/02 Hagen		new function
%  09/18/03 Hagen		change output amplitude from 200 to 2000

WaveZeroPad = [800 500];

if nargin < 3
    chanTapWeights = [1 0 0 0 .23];
end
if nargin < 2
    snr = 30;
end;    

%---- model the channel with FIR filter ----------------
rx = filter( chanTapWeights, 1, tx ); 
len = length(rx);


%---- set noise level based on specified SNR ---------------
rxPackets = rx( WaveZeroPad(1) : len-WaveZeroPad(2) ).^2;
signalPwr = sum(rxPackets)./length(rxPackets);

normNoise = randn(size(rx));
noisePwr = sum(normNoise.^2)./len;
    
noiseAmp = sqrt(signalPwr/noisePwr * 10.^(-snr/10));
noise = noiseAmp*normNoise;

rec = (rx+noise) ./ std(rx+noise) * 2000;


