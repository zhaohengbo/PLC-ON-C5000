function preamble = ofdmpreamble;
%  find a good preamble sequence for a powerline modem
%
%
diagPlot = 1;

Fs = 256;		%kHz
FFTlen = 256;
carrierBand = [10 : 94];
carrierLen = 85;
QAM = [1+0j  0+1j  -1+0j  0-1j];
%ph = [0:31]/32*2*pi;
%QAM = cos(ph)+j*sin(ph);

carriers = zeros(FFTlen,1);
numPhase = length(QAM);
wavepwr = zeros(carrierLen,numPhase);

for k = 1 : carrierLen,
%for k = carrierLen : -1 : 1,
    for ph = 1:numPhase
        carriers(carrierBand(k)) = QAM(ph);
        carriers(end+2-carrierBand(k)) = conj(QAM(ph));
        tx = real(ifft(carriers)) * FFTlen;            % modulate the signal 
        rms = sqrt(sum( tx.^2 )./ FFTlen); 
        peak = max(abs(tx));
        wavepwr(k,ph) = peak ./ rms;
    end;
    [mm,ph] = min( wavepwr(k,:) );

    carriers(carrierBand(k)) = QAM(ph);
    carriers(end+2-carrierBand(k)) = conj(QAM(ph));
end;

preamble = carriers(carrierBand);

disp('const iCplx PreambleArray[CARRIER_LEN] =')
disp(sprintf('\t{\t%4.0f,%4.0f,\t\t//%3.0f', real(preamble(1)), imag(preamble(1)), 0 ))
for( n = 2 : length(preamble) )
	disp(sprintf('\t\t%4.0f,%4.0f,\t\t//%3.0f', real(preamble(n)), imag(preamble(n)), n-1 ))
end
disp(sprintf('\t};'))

if diagPlot == 1
    tx = real(ifft(carriers))*FFTlen;       % modulate the signal 
    t = [1 : length(tx)]/Fs;				% msec
    rms = sqrt(sum( tx.^2 )./ FFTlen); 
    peak = max(abs(tx));
    
    figure(1);
    subplot(2,1,1);
    plot( t, tx );  
    handt2 = title(sprintf('pk/rms power = %.1f', peak ./ rms ));
    %set(handt2, 'units', 'normalized', 'position', [ .5  .8  0])
    
    subplot(2,1,2);
    plot(wavepwr,'+')
    
end

