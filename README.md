# dds-vfo
This is my implementation of a VFO based on an AD9850 which is controlled by an Arduino Nano v3. The VFO is used to pimp my T20P QRP transceiver.

The hardware desgin of the VFO is based on [Richard/AD7C's AD9850 DDS VFO](http://www.ad7c.com/projects/ad9850-dds-vfo/). 

The software was written from scratch. It uses a different layout in the LC display. It can be remote controlled via the serial port to set and get the frequency and the step size.

The T20P is a QRP transceiver kit for 20m that was produced in the 1990s by [Siegried Hari/DK9FN](http://www.hari-ham.com). It uses an IF of 10700kHz.

* Frequency: 14000kHz to 14350kHz (+ 10700kHz IF shift)
* Tuning step sizes: 10Hz, 50Hz, 100Hz, 250Hz, 500Hz, 1kHz, 2k5Hz, 5kHz, 10kHz, 100kHz

## Remote Control via Serial Port

Parameters: 9600Bd, 8 databits, 1 stopbit, no parity

Commands:

* `f`: decrease the frequency by step-size
* `F`: increase the frequency by step-size
* `s`: decrease the step-size
* `S`: increase the step-size
* `?`: get the current frequency in Hz
* `14nnnnnn`: set the current frequency 14nnnnnn in Hz

A change of the frequency always returns the current frequency.

