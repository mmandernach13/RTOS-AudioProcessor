# RTOS-AudioProcessor

This code is intended for Texas Instrument's EZDSPC5535 DSP developement board. It requires Code Composer Studio v6 to run correctly along with a gel file to configure the clock system. The program also requires DSP BIOS to be installed and several of the dsplib assembly files to make certain operations more efficient. It configures the board to accept an audio input via the audio input jack and perform various operations to it. It is important to note, that not all the files required to run this are in this repository, only the ones I derived or modified.

## Functions
Once audio is playing through the jack the user should see the LCD screen begin showing the audio's FFT (DC on far left, upper limit determined by sample rate) and the blue LED blinking. Then with the SW2 input, there are 4 possible output audio states indicated by the LEDs:
- [No LED]: No audio filtering
- [Green LED]: Lowpass audio filter
- [Red LED]: Highpass audio filter
- [Yellow LED]: Achieved by holding SW2. Thus enabling the CVSD encoder/decoder
