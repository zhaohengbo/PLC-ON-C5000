function build(option)
%  Build the PLC modem code for Matlab
%
%
%  Change History
%	28Feb03	Hagen	Rewrote so that args are on separate lines
%					and removed case switch for hard or soft
%					decode (this is now handled by a #define
%   01May03 Hagen   updated code filenames and added case for alt compiles


args = {
		    ' -g -output modem',
		    ' -DVCPP=1',
		    ' mex_modem.c', 
		    ' transmit.c', 
		    ' agc.c', 
		    ' preDet.c',
		    ' dataDet.c',
		    ' viterbi.c',
		    ' ofdm_fft.c',
		    ' global.c' 
		    ' AFE.c' 
		    ' diag.c' 
        };

if nargin > 0
	switch option
	case 1
		args = {
		    ' -g -output modem',
		    ' -DVCPP=1',
		    ' mex_modem.c', 
		    ' transmit.c', 
		    ' agc.c', 
		    ' preDet.c',
		    ' dataDet.c',
		    ' viterbi.c',
		    ' ofdm_fft.c',
		    ' global.c' 
		    ' AFE.c' 
		    ' diag.c' 
		};
	end
end



arg = 'mex';
for( n = 1 : length(args) )
    arg = [arg args{n}];
end

eval( arg )
