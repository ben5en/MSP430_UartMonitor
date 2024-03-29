# MSP430_UartMonitor

* This project shows the possibility to view and change variables via a serial
interface (UART) during the program runtime. 
* All globally visible variables can be entered and monitored as usual in the "Expression window" in debug mode.
* A software routine provided by TI is used. This works with addresses of the variables.

* Unfortunately in many cases the backchannel Uart (you'll find them on various launchpads) function is too slow. Therefore, it is recommended to use an external serial to USB adapter.

![Alternativtext](https://github.com/ben5en/MSP430_UartMonitor/blob/master/MSP430FR2433_UartMonitor.jpg "Hardware")

* Source: http://processors.wiki.ti.com/index.php/ProgramModelUart_GuiComposer
