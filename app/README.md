# Github organization

Inside the github there are three folders and two files.

The two files are:
- *certificati.txt* : this file contains the certificates used to connect to *HiveMQTT*
- *signal_generator_serial.py* : this file is used to generate an arbitrary sound signal

The three folders are:
- *app* : this folder contains the actual code used by the esp32
- *ina_consuption* : this folder contains a snippet of code used to measure the power consuption
- *other files* : this folder contains other useful files or the measurements of the performances

# General info

I used an esp32-v2 for developing, testing and measuring performances (expect fot power consuption, see below).

I used my smartphone hotspot (Android) as wifi as well as the PC one (Windows 11 based).

I soldered and used a jack-circuit to transform a digital sinusoid generated from my PC to a analog signal that could be sent to the pins of the esp.

# Code structure

Ignoring the classic file structure used by esp-idf projects and vscode, we will focus on the main folder and the managed_components.

Inside the managed_components folder, we can find the esp-dsp component, which we use for calculating the FFT.

Inside the main folder we find:
- *main.h* : inside this file, we find common #include directives, the definition of a common struct and few shared variables

- *main.c* : this file is the start of the program (so also FFT and wifi initialization) as well as the definition of *regular_task*, which is the task which actually set the loop of the program.

- *measure.c* : this file contains the code which gather the measurements from the adc.
It has two modes, one which samples at the frequency of 1 ms for 2 seconds and then kill itself. This mode is used for doing the FFT.
The loop mode, instead, samples at a frequency deduced by the FFT.
To allow to delay the task for 1 ms, I needed to modify the configuration of FreeRTOS using the idf.py menuconfig command and set the ticks from 100 to 1000 for second.

- *fft.c* : this code it used to initialize and perform the FFT.
The code baseline was taken by the ESP-DSP documentation and then modified.
The treshold for the Z score has been decided experimentally.

- *wifi.c* : this file contains the code for initialize, set and handle wifi communication. The code was developed by a base found on the internet which was later modified.
srcs: https://medium.com/@fatehsali517/how-to-connect-esp32-to-wifi-using-esp-idf-iot-development-framework-d798dc89f0d6 and ESP_IDF documentation

- *mqtt.c* :  this file contains the code for initialize, set and handle mqtt protocol. The code was developed by a base found on the internet which was later modified.
In particular, this code contains the loop which gather the measurements and sent it to the *HiveCloud*.
srcs: https://medium.com/@fatehsali517/how-to-code-mqtt-on-esp-idf-from-scratch-fe6bc238330 and ESP_IDF documentation

- *certificate.h* : it contains the certificate stored in bytes to use aside credentials to connect to *HiveCloud*.

- *time.c* : a small file used to evaluate performances.

# Program workflow

Once started:

- The *main function* set wifi and fft.

- Then it starts the *regular_task*, which in turn starts a *measure_task* without loop (which oversample at the maximum frequency of 1 ms for 2 sec).

- Once gathered the measurements using a StreamBuffer, it passes them to the FFT functions.

- From them, the *regular_task* gathers the maximum frequency and deduce the informations to create a stable loop which samples at the max frequency*2.

- It then starts a new *measure_task* with those informations and loop.

- Finally, it starts the *mqtt_task* and passes to it a StreamBuffer connected to the newest *measure_task*.

- It then kills itself.

- Hence, in the SO there are only the measure and the *mqtt_task*.

- The first samples and fill the StreamBuffer.

- The *mqtt_task* is blocked until the StreamBuffer is full, then calculate the average of the measurements gathered on the last 2 secs.

- The *mqtt_task* sent it to the *HiveMQTT* cloud.

# How to set the system

It suffice to: 
- write the desired output (by changing frequency and the equation of the waweform) into the *signal_generator_serial.py*

- connect the jack-circuit to the pc, then the ground, vcc and the remaining pin to the related pins of the esp32 (use ADC1 0 channel).

- change the name and password of the wifi connection inside the wifi.c file to match those of your router.

- create an account for *HiveCloud* and set a pair of credentials which you have to copy inside the mqtt.c file. Also, set the url of your cluster.

- follow these instructions to setup a certificate: 
    - from terminal -> openssl s_client -connect <SOMETHING_TO_PUT_HERE>.s1.eu.hivemq.cloud:8883 -showcerts > <CERTIFICATE_NAME>.txt
    - save the second certificate inside the file in <CERTIFICATE_NAME>.pem
    - from terminal -> xxd -i <CERTIFICATE_NAME>.pem > <CERTIFICATE_PEM>.h
    - change the name of the array in cert_pem and the int in cert_pem_len
    - copy the file in the main folder of the app

- play the *signal_generator_serial.py*

- build, flash and monitor the app for the esp32

- enjoy!

# Technical Discussion

## Oversampling and maximum frequency

The oversampling frequency is set to 1 ms because, since it is needed to gather samples for a long period in order to capture low frequencies, setting a frequency too high would cause a huge memory utilization which would make the program unusable on the esp.

Moreover, in order to augment the frequency, it would be needed to increase the FreeRTOS ticks even more, which would cause the power consumption and latency to increase beacuse of more clock interrupts.

Furthermore, a high frequency means also that the measure task is constantly up running, which would trigger the watchdog and thus undermin the correctness of our measurements.

Finally, with this setup we can detect consistently a frequency up to 500hz, which is more than enough for most applications.

## Power consumption (plus bonus points)

I measured the consumption of the system during sampling at the maximum frequency (500hz), 300hz, 100hz and 10hz.

To capture the measurements I used a Ina219 paired with an esp32-v3 since I didn't have Ina of my own. Paired with them, I used the Better Serial Plotter export function during the application loop, which returns the last 20 sec of measurements.

The results were, respectively: 286.43 mW , 300.28 mW, 286.92 mW, 283.23 mW.

Surprinsigly, the power consuption did not appear to change or, better, it is probable external factors (like the cables, the Ina precision, wifi keep-alive packets and so on) impeded a precise consuption measurements.

### Failed light sleep

I tried to implement a light sleep mode to try to get better power consuptions, but due to the lack of time I was unable to do it correctly.

Few snippets of code can be found in the *other files* folder.

## Latency of MQTT packets

To measure the latency, I followed this approach:

- subscribed to the voltage topic trough the web-client of *HiveCloud* using my credentials.

- I synchronized the esp with a sntp server to get the correct current time.

- printed the timestamp in millisecond just before sending a packet to the *HiveCloud*.

- save the timestamp made avaiable for each message received on *HiveCloud* (it is get by default)

- gather a suitable number of samples

- calculate the differences between the esp timestamp and the Hive timestamps

- compute the average

The average latency to receive a packet from the ESP trough MQTT is then: 227,25 ms

Few example of misurations can be found in the *calcolo_latenza_dimensione.txt* file

## Dimension of MQTT packets

To measure the dimension, I followed this approach:

- connect my Windows Pc to the hotspot of my phone.

- creating an hotspot from the PC a connecting the esp to it.

- using Wireshark to gather packets on the wifi

- deduce the ip of the esp

- filter as such: <code>tcp.port==8883 && ip.src==192.168.226.93</code> (that was the ip of my esp)

- studying the packets and notice that the vast mayority of non-ACK messages (so tagged as Application Data) were of the costant size of 104 bytes.
Moreover, they were received at the same rate the esp32 sent its packages.
The other non-ACK packets are probably MQTT events and keep-alive messages.

Hence, the size of a MQTT packet is 104 bytes.

Few example of misurations can be found in the *Pacchetti.png* screenshoot.