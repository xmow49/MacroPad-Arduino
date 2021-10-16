#include <Arduino.h>
#include <config.h>
#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h>
#include <EEPROM.h>

#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>

#define FONT2 Cooper19
#define FONT Adafruit5x7

bool textScrolling = 0;        // Store the state of text scrolling
String screenTxt = "MacroPad"; // Text display on the screen
uint32_t tickTime = 0;         // for the scrolling text

struct EncoderConf // Prepare encoders config
{
  byte mode;
  short value[3];
};

struct KeyConf // Prepare keys config
{
  byte mode;
  short value[3];
};

EncoderConf encoderConfig[3][profileCount - 1]; // Create the encoder config with 3 encoders
KeyConf keyConf[6][profileCount - 1];           // Create the key config with 6 keys

volatile int encodersPosition[3] = {50, 50, 50}; // temp virtual position of encoders
int encodersLastValue[3] = {50, 50, 50};         // Value of encoders

String serialMsg; // String who store the serial message

const byte keysPins[6] = {key0Pin, key1Pin, key2Pin, key3Pin, key4Pin, key5Pin};                                                                                   // Pins of all keys
const byte encodersPins[9] = {encoderA0Pin, encoderB0Pin, encoderKey0Pin, encoderA1Pin, encoderB1Pin, encoderKey1Pin, encoderA2Pin, encoderB2Pin, encoderKey2Pin}; // Pins of all encodes

SSD1306AsciiAvrI2c oled; // This is the Oled display
TickerState state;       // Ticker to run text scrooling

const byte repeatDelay = 5;
unsigned long currentMillis;

bool keyPressed[6];
bool encoderPressed[3];         // current state of encoder key
bool selectProfileMode = false; // If is true, all function are disabled, and keys do the profile selection
bool macropadNormalMode = true; // normal mode

volatile long encoderMillis;

byte currentProfile = 0;

String getArgs(String data, char separator, int index)
{
  // This Function separate String with special characters
  volatile int found = 0;
  volatile int strIndex[] = {0, -1};
  volatile int maxIndex = data.length() - 1;

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

void encoder(byte encoder) // function for the encoder interrupt
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  volatile byte pin;
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
    if (digitalRead(pin) == 0) // check the rotation direction
    {
      encodersPosition[encoder] = encodersLastValue[encoder] - 10; // anti-clockwise
    }
    else
    {
      encodersPosition[encoder] = encodersLastValue[encoder] + 10; // clockwise
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder0() // function call by the interrupt, and redirect to the main encoder fonction
{
  encoder(0);
}

void encoder1() // function call by the interrupt, and redirect to the main encoder fonction
{
  encoder(1);
}

void encoder2() // function call by the interrupt, and redirect to the main encoder fonction
{
  encoder(2);
}

void saveToEEPROM() // Save all config into the atmega eeprom
{
  Serial.println("-----Save to EEPROM-----");

  volatile short eepromAddress = 0;
  for (byte i = 0; i < 6; i++)
  {
    Serial.print("Key");
    Serial.print(i);
    Serial.print(" ");

    Serial.print(keyConf[i][currentProfile].mode);
    EEPROM.put(eepromAddress, keyConf[i][currentProfile].mode);
    eepromAddress += sizeof(keyConf[i][currentProfile].mode);

    Serial.print(":");
    Serial.print(keyConf[i][currentProfile].value[0]);
    EEPROM.put(eepromAddress, keyConf[i][currentProfile].value[0]);
    eepromAddress += sizeof(keyConf[i][currentProfile].value[0]);

    Serial.print(",");
    Serial.print(keyConf[i][currentProfile].value[1]);
    EEPROM.put(eepromAddress, keyConf[i][currentProfile].value[1]);
    eepromAddress += sizeof(keyConf[i][currentProfile].value[1]);

    Serial.print(",");
    Serial.println(keyConf[i][currentProfile].value[2]);
    EEPROM.put(eepromAddress, keyConf[i][currentProfile].value[2]);
    eepromAddress += sizeof(keyConf[i][currentProfile].value[2]);
  }

  for (byte i = 0; i < 3; i++)
  {
    Serial.print("Encoder");
    Serial.print(i);
    Serial.print(" ");

    // Save Mode
    Serial.print(encoderConfig[i][currentProfile].mode);
    EEPROM.put(eepromAddress, encoderConfig[i][currentProfile].mode);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].mode);

    // Save value 1
    Serial.print(":");
    Serial.print(encoderConfig[i][currentProfile].value[0]);
    EEPROM.put(eepromAddress, encoderConfig[i][currentProfile].value[0]);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].value[0]);

    // Save value 2
    Serial.print(",");
    Serial.print(encoderConfig[i][currentProfile].value[1]);
    EEPROM.put(eepromAddress, encoderConfig[i][currentProfile].value[1]);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].value[1]);

    // Save value 3
    Serial.print(",");
    Serial.println(encoderConfig[i][currentProfile].value[2]);
    EEPROM.put(eepromAddress, encoderConfig[i][currentProfile].value[2]);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].value[2]);
  }
}

void readEEPROM() // Read all config from the atmega eeprom and store it into the temp config
{
  Serial.println("-----Read from EEPROM-----");
  volatile short eepromAddress = 0;
  for (byte i = 0; i < 6; i++)
  {
    Serial.print("Key");
    Serial.print(i);
    Serial.print(" ");

    EEPROM.get(eepromAddress, keyConf[i][currentProfile].mode);
    Serial.print(keyConf[i][currentProfile].mode);
    eepromAddress += sizeof(keyConf[i][currentProfile].mode);

    Serial.print(":");
    EEPROM.get(eepromAddress, keyConf[i][currentProfile].value[0]);
    Serial.print(keyConf[i][currentProfile].value[0]);
    eepromAddress += sizeof(keyConf[i][currentProfile].value[0]);

    Serial.print(",");
    EEPROM.get(eepromAddress, keyConf[i][currentProfile].value[1]);
    Serial.print(keyConf[i][currentProfile].value[1]);
    eepromAddress += sizeof(keyConf[i][currentProfile].value[1]);

    Serial.print(",");
    EEPROM.get(eepromAddress, keyConf[i][currentProfile].value[2]);
    Serial.println(keyConf[i][currentProfile].value[2]);
    eepromAddress += sizeof(keyConf[i][currentProfile].value[2]);
  }
  for (byte i = 0; i < 3; i++)
  {
    Serial.print("Encoder");
    Serial.print(i);
    Serial.print(" ");

    // Get Mode
    EEPROM.get(eepromAddress, encoderConfig[i][currentProfile].mode);
    Serial.print(encoderConfig[i][currentProfile].mode);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].mode);

    // Get Value 1
    Serial.print(":");
    EEPROM.get(eepromAddress, encoderConfig[i][currentProfile].value[0]);
    Serial.print(encoderConfig[i][currentProfile].value[0]);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].value[0]);

    // Get Value 2
    Serial.print(":");
    EEPROM.get(eepromAddress, encoderConfig[i][currentProfile].value[1]);
    Serial.print(encoderConfig[i][currentProfile].value[1]);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].value[1]);

    // Get Value 3
    Serial.print(":");
    EEPROM.get(eepromAddress, encoderConfig[i][currentProfile].value[3]);
    Serial.print(encoderConfig[i][currentProfile].value[3]);
    eepromAddress += sizeof(encoderConfig[i][currentProfile].value[3]);
  }
}

void scrollText() // function to scoll the text into the display
{
  if (tickTime <= millis())
  {
    tickTime = millis() + 30;

    if (textScrolling)
    {
      int8_t rtn = oled.tickerTick(&state);

      if (rtn <= 0)
      {
        oled.tickerText(&state, screenTxt);
      }
    }
  }
}

void centerText(String text) // Display the text in the center of the screen
{
  int str_len = text.length();
  char char_array[str_len];
  text.toCharArray(char_array, str_len);
  size_t size = oled.strWidth(char_array);

  oled.clear();
  oled.setFont(FONT);
  oled.set2X();

  oled.setCursor((oled.displayWidth() - size) / 2, ((oled.displayRows() / 2) - 2)); // Center
  oled.println(screenTxt);
  textScrolling = false;
}

void setRGB(byte r, byte g, byte b) // Set rgb value for LEDs
{
  analogWrite(ledR, r);
  analogWrite(ledG, g);
  analogWrite(ledB, b);
}

void selectProfile()
{
  screenTxt = "Profil " + (currentProfile + 1);
  int str_len = screenTxt.length();
  char char_array[str_len];
  screenTxt.toCharArray(char_array, str_len);
  size_t size = oled.strWidth(char_array);

  centerText(screenTxt);
  textScrolling = false;

  for (byte i = 0; i <= 5; i++)
  {
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Start");
  setRGB(255, 0, 0);

  Serial.println("PinMode:");
  // Keys
  for (byte i = 0; i < 6; i++)
  {
    pinMode(keysPins[i], INPUT);
    Serial.print("Key:");
    Serial.println(keysPins[i]);
  }
  // Encoders
  for (byte i = 0; i < 3; i++)
  {
    pinMode(encodersPins[i], INPUT);
    Serial.print("Encoder:");
    Serial.println(encodersPins[i]);
  }
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  delay(1000);

  setRGB(0, 255, 0);

  Serial.println("Setup Interrupts");
  //------------Interrupts----------
  attachInterrupt(digitalPinToInterrupt(encoderA0Pin), encoder0, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA1Pin), encoder1, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA2Pin), encoder2, LOW);

  Serial.println("Setup HID");
  Consumer.begin(); // Start HID

  Serial.println("Setup Display");

  Wire.begin();
  oled.begin(&Adafruit128x32, 0x3C);

  Serial.println("Read EEPROM");
  readEEPROM();

  delay(1000);
  setRGB(255, 0, 0);

  centerText("Macropad");
  centerText("Macropad");
  delay(1000);
}

void loop()
{
  if (selectProfileMode)
  {
    selectProfile();
  }
  else if (macropadNormalMode) //Default mode
  {

    // Check Keys
    for (byte i = 0; i < 6; i++)
    {
      if (digitalRead(keysPins[i]))
      {
        if (keyPressed[i])
        { // if the key is already pressed
        }
        else
        {
          Serial.print("Key" + String(i));
          if (keyConf[i][currentProfile].mode == 0) // System Action
          {
            Consumer.write(keyConf[i][currentProfile].value[0]);
            // Serial.println(keyConf[i][currentProfile].value[0]);
            currentMillis = millis() / 100;
            // while (digitalRead(keysPins[i]))
            // {
            //   if (currentMillis + repeatDelay <= millis() / 100)
            //   {
            //     Consumer.write(keyConf[i][currentProfile].value[0]);
            //     delay(50);
            //   }
            // }
          }
          else if (keyConf[i][currentProfile].mode == 1) // Key Combination
          {

            for (byte j = 0; j < 3; j++) // for each key
            {
              if (keyConf[i][currentProfile].value[j] == 0 && keyConf[i][currentProfile].value[j] != 60) // if no key and key 60 (<) because impoved keyboard
              {
              }
              else if (keyConf[i][currentProfile].value[j] <= 32 && keyConf[i][currentProfile].value[j] >= 8) // function key (spaces, shift, etc)
              {
                Keyboard.press(keyConf[i][currentProfile].value[j]);
                Serial.print("Press ");
                Serial.println(keyConf[i][currentProfile].value[j]);
              }
              else if (keyConf[i][currentProfile].value[j] <= 90 && keyConf[i][currentProfile].value[j] >= 65) // alaphabet
              {                                                                                                // ascii normal code exept '<' (60)
                Keyboard.press(keyConf[i][currentProfile].value[j] + 32);                                      // Normal character + 32 (ascii Upercase to lowcase)
                Serial.print("Press ");
                Serial.println(keyConf[i][currentProfile].value[j] + 32);
              }
              else if (keyConf[i][currentProfile].value[j] <= 57 && keyConf[i][currentProfile].value[j] >= 48) // numbers
              {
                Keyboard.press(keyConf[i][currentProfile].value[j]); // Normal number
              }
              else
              {
                Keyboard.press(KeyboardKeycode(keyConf[i][currentProfile].value[j] - 100)); // Improved keyboard
                Serial.print("Press ");
                Serial.println(keyConf[i][currentProfile].value[j] - 100);
              }
            }

            // while (digitalRead(keysPins[i]))
            // {
            //   if (currentMillis + repeatDelay <= millis() / 100)
            //   {
            //     delay(50);
            //   }
            // }
          }
        }

        keyPressed[i] = true;
      }
      else
      {
        keyPressed[i] = false;
        Keyboard.releaseAll();
      }
    }

    // Check Encoders
    for (byte i = 0; i < 3; i++) // For every encoder
    {
      if (encodersPosition[i] != encodersLastValue[i]) // If Encoder New Value
      {
        if (encodersPosition[i] < encodersLastValue[i]) // Encoder ClockWise Turn
        {

          Serial.print("Encoder" + String(i) + ":UP");

          if (encoderConfig[i][currentProfile].mode == 0 && encoderConfig[i][currentProfile].value[0] == 0) // If is a System Action AND master volume selected
          {
            Consumer.write(MEDIA_VOL_UP);
          }
          else if (encoderConfig[i][currentProfile].mode == 0 && encoderConfig[i][currentProfile].value[0] == 1) // If is a System Action AND master volume selected
          {
            Consumer.write(HID_CONSUMER_FAST_FORWARD);
          }
          else if (encoderConfig[i][currentProfile].mode == 1) // If is a key action
          {
            Keyboard.write(encoderConfig[i][currentProfile].value[0]); // get the ascii code, and press the key
          }
        }
        else // Encoder Anti-ClockWise Turn
        {
          Serial.print("Encoder" + String(i) + ":DOWN");

          if (encoderConfig[i][currentProfile].mode == 0 && encoderConfig[i][currentProfile].value[0] == 0)
          {
            Consumer.write(MEDIA_VOL_DOWN);
          }
          else if (encoderConfig[i][currentProfile].mode == 0 && encoderConfig[i][currentProfile].value[0] == 1) // If is a System Action AND master volume selected
          {
            Consumer.write(HID_CONSUMER_REWIND);
          }
          else if (encoderConfig[i][currentProfile].mode == 1) // If is a key action
          {
            Keyboard.write(encoderConfig[i][currentProfile].value[1]); // get the ascii code, and press the key
          }
        }
        encodersLastValue[i] = encodersPosition[i];
      }
    }
  }

  if (!digitalRead(encoderKey1Pin)) // When encoder 1 key is pressed
  {

    if (encoderPressed[1])
    { // if the encoder is already pressed
      // wait
      Serial.println("Already press");
    }
    else
    { // first press of the encoder
      Serial.println("First press");
      encoderPressed[1] = true;
      encoderMillis = millis();
    }
    if (encoderMillis + 3000 < millis())
    {
      // 3 second after the begin of the press
      Serial.println("3S --> profile set");
      // set profile menu
      selectProfileMode = true;
    }
  }

  if (digitalRead(encoderKey1Pin)) // When encoder 1 key is relese
  {

    // Do the action
    encoderPressed[1] = false;
  }

  if (!digitalRead(encoderKey0Pin) && !digitalRead(encoderKey1Pin) && !digitalRead(encoderKey2Pin)) // install the software
  {
    Consumer.write(0x223); // open default web Browser
    delay(200);
    Keyboard.press(KEY_LEFT_CTRL); // focus the url with CTRL + L
    Keyboard.press(KEY_L);
    delay(10);
    Keyboard.releaseAll();
    delay(100);
    Keyboard.print("https://github.com/xmow49/MacroPad-Software/releases"); // enter the url
    delay(100);
    Keyboard.write(KEY_ENTER); // go to the url
  }

  if (Serial.available() > 0)
  {
    serialMsg = Serial.readString();
    serialMsg.trim();

    String command = getArgs(serialMsg, ' ', 0);
    short arg[5];
    for (byte i = 0; i < 5; i++)
    {
      arg[i] = getArgs(serialMsg, ' ', i + 1).toInt();
    }
    /*
    Serial.println(arg[0]); //n encoder/key
    Serial.println(arg[1]); //mode
    Serial.println(arg[2]); //Value 1
    Serial.println(arg[3]); //Value 2
    Serial.println(arg[4]); //Value 3
    */
    if (command == "ping")
    {
      Serial.println("pong");
    }
    else if (command == "set-key")
    {
      keyConf[arg[0]][currentProfile].mode = arg[1]; // Save Mode
      for (byte i = 0; i < 3; i++)                   // foreach value in the command
      {
        keyConf[arg[0]][currentProfile].value[i] = arg[i + 2]; // Save Values
      }

      Serial.println("OK");
    }

    else if (command == "set-encoder") // Set encoder action
    {
      encoderConfig[arg[0]][currentProfile].mode = arg[1]; // Get the mode and save it in the config
      for (byte i = 0; i < 3; i++)                         // for all values, save it in the config
      {
        encoderConfig[arg[0]][currentProfile].value[i] = arg[i + 2];
      }
      Serial.println("OK");
    }
    else if (command == "set-text")
    {
      String txt = getArgs(serialMsg, '"', 1);
      txt.remove(txt.length() - 1);
      screenTxt = txt;
      int str_len = screenTxt.length();
      char char_array[str_len];
      screenTxt.toCharArray(char_array, str_len);
      size_t size = oled.strWidth(char_array);

      size = 150;
      if (size > 128)
      {
        oled.clear();
        oled.tickerInit(&state, FONT, 1, true, 0, oled.displayWidth());
        textScrolling = true;
      }
      else
      {
        centerText(screenTxt);
        textScrolling = false;
      }

      Serial.println("OK");
    }

    else if (command == "get-config") // Get config command
    {
      for (byte i = 0; i < 6; i++) // for all keys, get the config and display it
      {
        Serial.print("Key");
        Serial.print(i);
        Serial.print(" ");
        Serial.print(keyConf[i][currentProfile].mode);
        Serial.print(" ");
        Serial.print(keyConf[i][currentProfile].value[0]);
        Serial.print(":");
        Serial.print(keyConf[i][currentProfile].value[1]);
        Serial.print(":");
        Serial.println(keyConf[i][currentProfile].value[2]);
      }
      for (byte i = 0; i < 3; i++) // for all encoders, get the config and display it
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.print(" ");
        Serial.print(encoderConfig[i][currentProfile].mode);
        Serial.print(" ");
        Serial.print(encoderConfig[i][currentProfile].value[0]);
        Serial.print(":");
        Serial.print(encoderConfig[i][currentProfile].value[1]);
        Serial.print(":");
        Serial.println(encoderConfig[i][currentProfile].value[2]);
      }
    }
    else if (command == "save-config") // save-config command
    {
      saveToEEPROM();
    }
    else if (command == "read-config") // read-config command
    {
      readEEPROM();
    }

    else if (command == "set-rgb")
    {
      setRGB(arg[1], arg[2], arg[3]);
      Serial.println("OK");
    }
    else if (command == "reset-config")
    {
      for (byte i = 0; i < 3; i++)
      {
        encoderConfig[i][currentProfile].mode = 0;
        for (byte j = 0; j < 3; j++)
        {
          encoderConfig[i][currentProfile].value[j] = 0;
        }
      }

      for (byte i = 0; i < 6; i++)
      {
        keyConf[i][currentProfile].mode = 0;
        for (byte j = 0; j < 3; j++)
        {
          keyConf[i][currentProfile].value[j] = 0;
        }
      }
      Serial.println("OK");
    }

    else
    {
      Serial.print(command);
      Serial.println(": command not found");
    }
  }

  scrollText();
  delay(10);
}
