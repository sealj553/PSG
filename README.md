# PSG
Sends data and commands from Raspberry Pi to Android Phone via Bluetooth

* Originally designed for sending text messages from phone when event is triggered on Raspberry Pi GPIO
* Sends "heartbeat" to immediately detect loss of bluetooth connection
* Android app created with Android Studio, RPi server written in C with bluetooth sockets, uses wiringPi for GPIO access
