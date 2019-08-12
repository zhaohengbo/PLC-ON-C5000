%  Run the PLC modem
%
%  Change History
%	28Feb03	Hagen	Changed to remove 'agc' call to modem.dll
%	13Jun03	Hagen	Added try/catch around mex receive call and added agc diag plots
%	23Jan04	Hagen	add switch for various data sources

dataSource = 1;

%---- create the transmitted signal ----------------------------------
parm = modem('parm');

switch dataSource
case 1
    udata =  round(256*rand(1,parm.NUM_USER_BYTES)-0.5);
case 2
    if ~exist('udata')
        udata =  round(256*rand(1,parm.NUM_USER_BYTES)-0.5);
    end
case 3
    udata = zeros(1,parm.NUM_USER_BYTES);
end

%disp('generating xmit waveform')
tx = modem('xmit', udata);
disp(sprintf('xmit waveform generated.  Power = %.3f', std(tx) ))

%---- model the channel --------------------------------------------
if ~exist('snr')
    snr = 25;
end

disp(sprintf('sim channel. SNR = %.1f', snr(1) ))
%ec = ofdm_chan( tx, snr(1), [1 0 0 0 .0001] );
rec = ofdm_chan( tx, snr(1) );

%---- decode the data -----------------------------------------------
disp('receiving signal')

try
    [err, rdata, rxagc] = modem('rec', rec); % do receive function
    disp(sprintf('parity checksum = %.0f', err))
catch
    disp(lasterr)
    rdata = [];
    rxagc = [];
end


%---- display the userData bytes -----------------------------
errCnt = 0;
for( n = 1 : min([length(rdata) length(udata)]) )
	if( udata(n) ~= rdata(n) )
		disp(sprintf('%3.0f   x%02X  x%02X', n-1, udata(n), rdata(n) ))
		errCnt = errCnt+1;
	end
	if( errCnt > 20 )
		break;
	end
end

return

%---- display packet detect diagnoistics -------------------------
if exist( 'snrDiag' )
    ix = find(abs(snrDiag(1,:)) > 2); 
    N = ix(end);
    
    figure(1);
    subplot(4,1,1)
    plot(snrDiag(1,1:N))
    ylabel('signal')
    
    subplot(4,1,2)
    plot(snrDiag(2,1:N))
    ylabel('pre count')
    
    subplot(4,1,3)
    plot(snrDiag(3,1:N))
    ylabel('agc gain')
    
    subplot(4,1,4)
    plot(snrDiag(4,1:N))
    ylabel('dd phase')
    xlabel('samples')
end    
    
