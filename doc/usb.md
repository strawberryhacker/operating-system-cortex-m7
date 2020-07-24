# USB host 

The USB host stack will be divided into different layers. First is the USBHC driver which will control and talk to the hardware. This layer is responsible for sending complete transactions, monitioring error conditions and reporting status or data back to the caller. 
