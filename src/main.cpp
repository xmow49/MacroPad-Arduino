#include <Arduino.h>
#include <pins.h>
#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h>
#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

//List of action
/*const String strAction[17] PROGMEM = {"MediaFastForward", "MediaRewind", "MediaNext", "MediaPrevious", "MediaStop", "MediaPlayPause",
                      "MediaVolumeMute", "MediaVolumeUP", "MediaVolumeDOWN",
                      "ConsumerEmailReader", "ConsumerCalculator", "ConsumerExplorer",
                      "ConsumerBrowserHome", "ConsumerBrowserBack", "ConsumerBrowserForward", "ConsumerBrowserRefresh", "ConsumerBrowserBookmarks"};
*/

//List of actions in Hex
const short hexAction[17] PROGMEM = {0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xCD,
                     0xE2, 0xE9, 0xEA,
                     0x18A, 0x192, 0x194,
                     0x223, 0x224, 0x225, 0x227, 0x22A};

//List of key current action
int keyAction[6] = {0, 0, 0, 0, 0, 0};

String encoderAction[] = {"", "", ""};
String encoderValue[] = {"", "", ""};

volatile int encodersPosition[3] = {50, 50, 50};
int encodersLastValue[3] = {50, 50, 50};

String serialMsg;

const int keysPins[6] = {key0, key1, key2, key3, key4, key5};

const int encodersPins[9] = {encoderA0, encoderB0, encoderKey0, encoderA1, encoderB1, encoderKey1, encoderA2, encoderB2, encoderKey2};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String getArgs(String data, char separator, int index)
{
  //Function for separate String with special characters
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

short getNumberArgs(String data, char separator)
{
  int nArgs = 0;
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex; i++)
  {
    if (data.charAt(i) == separator)
    {
      nArgs++;
    }
  }
  return nArgs;
}

void encoder(byte encoder)
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  volatile int pin;
  switch (encoder)
  {
  case 0:
    pin = encodersPins[1];
    break;
  case 1:
    pin = encodersPins[4];
    break;
  case 2:
    pin = encodersPins[7];
    break;

  default:
    return;
    break;
  }
  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(pin) == 0)
    {
      encodersPosition[encoder] = encodersLastValue[encoder] - 10;
    }
    else
    {
      encodersPosition[encoder] = encodersLastValue[encoder] + 10;
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder0()
{
  encoder(0);
}

void encoder1()
{
  encoder(1);
}

void encoder2()
{
  encoder(2);
}

void getstrAction()
{
  volatile byte key = 0;

  int sizeKeyAction = sizeof(keyAction) / sizeof(int *);
  int sizeHexAction = sizeof(hexAction) / sizeof(int *);
  for (byte i = 0; i < sizeKeyAction; i++)
  {
    for (byte i = 0; i < sizeKeyAction; i++)
    {
      for (byte i = 0; i < sizeHexAction; i++)
      {
        if (hexAction[i] == keyAction[key])
        {
          Serial.print("Key");
          Serial.print(key);
          Serial.print(": ");
          Serial.println(hexAction[i]);
          break;
        }
      }
      key++;
    }
  }
}

void saveToEEPROM()
{
  Serial.println("-----Save to EEPROM-----");
  volatile int key = 0;
  volatile short eepromAddress = 0;
  for (byte i = 0; i < sizeof(keyAction) / sizeof(int *); i++)
  {
    Serial.print("Save to EEPROM: Key");
    Serial.print(key);
    Serial.print(": ");
    Serial.println(keyAction[i]);
    EEPROM.put(eepromAddress, keyAction[i]);
    eepromAddress += sizeof(keyAction[i]);
    key++;
  }

  for (byte i = 0; i < sizeof(encoderAction) / sizeof(encoderAction[i]); i++)
  {
    Serial.print("Save to EEPROM: Encoder");
    Serial.print(encoderAction[i]);
    Serial.print(": ");
    Serial.println(encoderValue[i]);

    EEPROM.put(eepromAddress, encoderAction[i]);
    eepromAddress += sizeof(encoderAction[i]);

    EEPROM.put(eepromAddress, encoderValue[i]);
    eepromAddress += sizeof(encoderValue[i]);
  }
}

void readEEPROM()
{

  volatile byte key = 0;
  volatile short eepromAddress = 0;
  volatile short action;
  int sizeKeyAction = sizeof(keyAction) / sizeof(int *);

  Serial.println("Read from EEPROM: Key");
  for (byte i = 0; i < sizeKeyAction; i++)
  {
    EEPROM.get(eepromAddress, action);
    Serial.print(key);
    Serial.print(": ");
    Serial.println(action);
    keyAction[i] = action;
    eepromAddress += sizeof(keyAction[i]);
    key++;
  }

  volatile int nencoder = 0;
  String strValue;
  String strAction;

  /*Serial.println("Read from EEPROM: Encoder");
  for (byte i = 0; i < 3; i++)
  {
    EEPROM.get(eepromAddress, strAction);

    Serial.print(eepromAddress);
    Serial.print(":");
    Serial.print(strAction);

    eepromAddress += sizeof(encoderAction[i]);
    EEPROM.get(eepromAddress, strValue);
    eepromAddress += sizeof(encoderValue[i]);

    encoderAction[i] = strAction;
    encoderValue[i] = strValue;
    Serial.print(strAction);
    Serial.print(": ");
    Serial.println(strValue);
    nencoder++;
  }*/
}

/*int getAction(String action)
{
  for (byte i = 0; i < sizeof(strAction); i++)
  {
    if (action == strAction[i])
    {
      Serial.print("Found: ");
      Serial.println(i);
      return hexAction[i];
      break;
    }
  }
  return 0;
}
*/

/*void setText(String txt, unsigned short size = 1, GFXfont *f = &FreeSans9pt7b, int x = 0, int y = 0, bool color = 1)
{
  display.setTextSize(size);
  display.setTextColor(color);
  display.setFont(f);
  display.setCursor(x, y);
  display.print(txt);
  display.display();
}*/

String screenTxt = "MacroPad";
int xTxt = display.width();
int xMin = -11 * screenTxt.length();

void scrollText()
{
  display.setTextSize(1);
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  unsigned int y = 0;
  if (y < 6 * screenTxt.length() + 6)
  {
    display.fillRect(0, 9, 128, 25, BLACK);
    display.setCursor(xTxt, 23);
    display.print(screenTxt);
    xTxt--;
    if (xTxt < xMin)
      xTxt = display.width();
    y++;
  }
  display.display();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Start");

  Serial.println("PinMode:");
  //Keys
  for (byte i = 0; i < sizeof(keysPins) / sizeof(int *); i++)
  {
    pinMode(keysPins[i], INPUT);
    Serial.print("Key:");
    Serial.println(keysPins[i]);
  }

  //Encoders
  for (byte i = 0; i < 3; i++)
  {
    pinMode(encodersPins[i], INPUT);
    Serial.print("Encoder:");
    Serial.println(encodersPins[i]);
  }

  Serial.println("Setup Interrupts");
  //------------Interrupts----------
  attachInterrupt(digitalPinToInterrupt(encoderA0), encoder0, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA1), encoder1, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA2), encoder2, LOW);

  Serial.println("Setup HID");
  Consumer.begin(); //Start HID

  Serial.println("Setup Display");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  Serial.println("Read EEPROM");
  readEEPROM();
  delay(1000);
}

const int repeatDelay = 5;
unsigned int currentMillis;

void loop()
{
  //Check Keys
  for (byte i = 0; i < sizeof(keysPins) / sizeof(int *); i++)
  {
    if (digitalRead(keysPins[i]))
    {
      Serial.print("Key ");
      Serial.print(i);
      Serial.println(" Pressed");
      Consumer.write(keyAction[i]);
      currentMillis = millis() / 100;
      while (digitalRead(keysPins[i]))
      {
        if (currentMillis + repeatDelay <= millis() / 100)
        {
          Consumer.write(keyAction[i]);
          delay(50);
        }
      }
    }
  }

  //Check Encoders
  for (byte i = 0; i < sizeof(encodersPosition) / sizeof(int *); i++)
  {
    if (encodersPosition[i] != encodersLastValue[i])
    {
      if (encodersPosition[i] < encodersLastValue[i])
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.println(":UP");

        Serial.println(encoderAction[i]);
        Serial.println(encoderValue[i]);

        if (encoderAction[i] == "system" && encoderValue[i] == "volume")
        {
          Consumer.write(MEDIA_VOL_UP);
        }
      }
      else
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.println(":DOWN");

        if (encoderAction[i] == "system" && encoderValue[i] == "volume")
        {
          Consumer.write(MEDIA_VOL_DOWN);
        }
      }
      encodersLastValue[i] = encodersPosition[i];
    }
  }

  if (Serial.available() > 0)
  {
    serialMsg = Serial.readString();
    serialMsg.trim();

    if (serialMsg.indexOf("ping") > -1)
    {
      Serial.println("pong");
    }

    String command = getArgs(serialMsg, ' ', 0);
    String arg1 = getArgs(serialMsg, ' ', 1);
    String arg2 = getArgs(serialMsg, ' ', 2);
    String arg3 = getArgs(serialMsg, ' ', 3);
    String arg4 = getArgs(serialMsg, ' ', 4);
    if (command == "set-key")
    {
      if (arg2 == "action")
      {
        int key = arg1.toInt();
        keyAction[key] = arg3.toInt();
      }
      else if (arg2 == "combination")
      {
        //short numberOfKey = getNumberArgs(serialMsg, ' ') - 3;
        int key = getArgs(serialMsg, ' ', 2).toInt();
        serialMsg.remove(0, 22);

        keyAction[key] = -1;
      }
    }

    else if (command == "set-encoder")
    {
      if (arg2 == "system")
      {
        int encoder = arg1.toInt();
        encoderAction[encoder] = arg2;
        delay(10);
        encoderValue[encoder] = arg3;
        Serial.println("OK");
      }
    }
    else if (command == "set-text")
    {
      if (arg1 == "text")
      {
        String txt = getArgs(serialMsg, '"', 1);
        txt.remove(txt.length() - 1);
        Serial.println(txt);
        screenTxt = txt;
        xTxt = display.width();
        xMin = -11 * screenTxt.length();
      }
    }

    else if (command == "get-config")
    {
      getstrAction();
    }
    else if (command == "save-config")
    {
      saveToEEPROM();
    }
    else if (command == "read-config")
    {
      readEEPROM();
    }
  }

  scrollText();
  delay(10);
}
