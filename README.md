# STM32_SINGLE-2-ONE_Wire
STM32 Single Wire(TM) mode interface to ONEWIRE(TM)

 After struggling a lot around never found nothing using Single Wire to interface Onewire without Hardware.
  Reading about Single wire it seemed ok but ST just provided Half duplex mode thru
   ST provide HAL_HalfDuplex_EnableTransmitter(), HAL_HalfDuplex_EnableReceiver() this way receiver is off when transmit and viceversa.
 Hacking Hal library I produced this:
