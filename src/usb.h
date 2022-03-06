#include "pins_arduino.h"

#ifdef USBSTA
 #define TEP_USB_VBUS_CONNECTED    (USBSTA == 3) 
 #define TEP_USB_VBUS_DISCONNECTED (USBSTA == 2) 
#endif


bool isOnUSBPower()
{
  #ifdef TEP_USB_VBUS_CONNECTED   
    return TEP_USB_VBUS_CONNECTED;
  #else 
   return false;
  #endif 
}


#ifdef UDADDR
 #define TEP_USB_ADDRESS (isOnUSBPower()?UDADDR:0)
#endif

#if defined(ADDEN) && defined(UDINT)   
 #define TEP_USB_ADDRESS_ESTABLISHED (_BV(ADDEN)== 0x80 && UDINT > 0 )
#endif

uint8_t getUSBaddress()
{
#ifdef TEP_USB_ADDRESS
  return TEP_USB_ADDRESS;
#else
  return 0;
#endif
}

bool isUSBDataEstablished()
{
#ifdef TEP_USB_ADDRESS_ESTABLISHED
  static uint8_t iLastAddress = 0;
  static uint8_t bUSBDataEstablished = false;

  // The address changes from 0 to something (byte) or increases
  // with one each time you plugin the device, when this is an USB host.
  // If the VBUS not connected, it returns always 0 (zero)
  uint8_t iAddress = getUSBaddress();
  if (iAddress > 0)
  {
    // Serial1.println( _BV(ADDEN), HEX );
    // Serial1.println( UDINT, HEX );

    if (TEP_USB_ADDRESS_ESTABLISHED)
    {
      // Serial1.println( iAddress );
      //  Need update? On USB Battery for example, address stays the same
      //  so it never perform an update and never reports there is a
      //  data connection established.
      if (iAddress != iLastAddress)
      {
        bUSBDataEstablished = true;
        iLastAddress = iAddress;
      }
    }
  }
  else
  {
    bUSBDataEstablished = false;
  }

  return bUSBDataEstablished;
#else
  return false;
#endif
}

