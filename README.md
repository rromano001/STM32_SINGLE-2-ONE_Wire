# STM32_SINGLE-2-ONE_Wire
STM32 Single Wire(TM) mode interface to ONEWIRE(TM)

New to GitHub, I try manage files and learn. Complete project is on release.zip other than Pictures
 Curent code work on DS18S20 DS1820, change readtempds18s20 to readtempds18b20 on main loop.
 
Checked a lot about HAL version, moved to F030 resulted in a mess about USART IRQ locking out everything.
 Changed code to LL USART DMA is now ready in superloop mode nonblocking mode.
 Complete example released for F030, see setup.
march 21 2020 Project opening (work in progress wait few day and I post full working project, actually hosted on a STM32F303RETX I plan provide small Lib for F030 too)

 After struggling a lot around never found nothing using Single Wire to interface Onewire without Hardware.
  Using microsecond delay OS can disrupt timing or block other task from execute.
  Using external hardware as many suggested waste RX pin can be used to do other task.
  A lot of code is on many sites, Best one appear to be Tilen Majerle one but again use both RX and TX pin.
  Another was using usart in DMA mode https://cnnblike.com/post/stm32-OneWire/, this intrigued me but code presented some issue and no example was provided.
 Modified to use Single wire I added as seen on commented code half duplex mode and enale.
 This way code was not working and transmitter never stop sending last byte sent.
 Reading a fresh new article got me some inspiration, so hacked HAl library producinh a full duplex RTX enable
 https://electronics.stackexchange.com/questions/484079/stm32-usart-1-wire-communication
   Reading about Single wire it seemed ok but ST just provided Half duplex mode thru
  HAL_HalfDuplex_EnableTransmitter(),
  HAL_HalfDuplex_EnableReceiver() this way receiver is off when transmit and viceversa.
  
 What is still missing is the full duplex of USART, USART IP is full duplex in nature and RX TX where simply disabled by register so the hack of both RX and TX channels enabled.
  
 Hacking Hal library I produced HAL_SnglWireFullDuplex_EnableRTX() that enable both receiver and transmitter on same single pin.

Test on simply attached to a Temperature sensor ds1820 non Parasitic power mode internal pullup resulted working fine.
Original example was using DMA but DMA failed in some strange way is under investigation.
 Temporarily switched to Interrupt mode for now, it is working.
 
 
 ---VDD---|             |---VDD--- (Ground to use Parasitic mode)
          |             |
     PA10 |-------------| DQ       (Add 4K7 to VDD to use parasitic mode)
          |             |
 ---VSS---|             |---VSS---
    uC                   DS Onewire
    
   This way just connect VDD VSS and DQ of sensor to STM32 TX configured as single wire.
   
