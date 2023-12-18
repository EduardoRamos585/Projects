
Sample_n = 'noisyconversation.wav';

[x, fs] = audioread(Sample_n);
figure(1),spectrogram(x,100),title('Spectrogram of Noisy Audio');
%From spectrogram, we observe that we need a lowpass filter to remove noise
k = linspace(-fs/2,fs/2,length(x));
X = fft(x);
X = X./max(X);
figure(2),plot(k,abs(fftshift(X))),title('DFT of Noisy Audio');
xlabel("Frequency: Hz")
ylabel("Magnitude: dB")
figure(3),plot(k,20*log10(abs(fftshift(X)))),title('log DFT of Noisy Audio');
xlabel("Frequency: Hz")
ylabel("Magnitude: dB")

% In the spectogram, we observe that our Ws is 0.34pi and Wp is 0.30pi
%From the log of DFT noisy audio our stopband gain is approx 64 dB and
% 29db for passband

[N,Wc]=buttord(0.30, 0.34, 29, 64);
[B,A]=butter(N,Wc);

[H,W] = freqz(B,A,(length(x)-1));
figure(4),plot(W/pi,abs(H)),title('Magnitude of Filter Response')
xlabel(" Frequency: rad/s")
ylabel("Magnitude : dB")


W = W.';
H = H.';
k2 = (fs/2)*[-fliplr(W) 0 W]/pi;
H2 = [fliplr(H) 1 H];


y = filter(B,A,x);
Y = fft(y);
Y = Y./max(Y);

figure(5),subplot(2,1,1),plot(k,abs(fftshift(X))),title('DFT Before Filtering'),hold on;
xlabel("Frequency: Hz")
ylabel("Magnitude: dB")
figure(5),subplot(2,1,1),plot(k2,abs(H2)),hold off;
xlabel("Frequency: Hz")
ylabel("Magnitude: dB")
figure(5),subplot(2,1,2),plot(k,abs(fftshift(Y))),title('DFT After Filtering'),hold on;
xlabel("Frequency: Hz")
ylabel("Magnitude: dB")
figure(5),subplot(2,1,2),plot(k2,abs(H2)),hold off;
xlabel("Frequency: Hz")
ylabel("Magnitude: dB")
figure(6),subplot(2,1,1),plot(x),title('Audio Before Filtering'),hold on;
xlabel("Time : s")
ylabel("Magnitude: dB")
figure(6),subplot(2,1,2),plot(y),title('Audio After Filtering');
xlabel("Time : s")
ylabel("Magnitude: dB")


output = 'Result.wav';
audiowrite(output,y,fs)


