#include <Arduino.h>
#include <config.h> //config file

#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h> //Keybord ++
#include <EEPROM.h>

// --- Oled Display ---
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans12pt7b.h>

#include <MemoryUsage.h>

String strIcon;
bool waitSerialIcon;
unsigned char msgCount;

bool textScrolling = false; // Store the state of text scrolling
short textX;                // Store the current X position of the text
String textOnDisplay;       // Store the current scolling text

struct EncoderConf // Prepare encoders configstructure
{
  byte mode;
  short value[3];
};

struct KeyConf // Prepare keys config structure
{
  byte mode;
  short value[3];
};

struct RGBConf // Prepare RGB config structure
{
  byte r;
  byte g;
  byte b;
};

unsigned char logoConf[PROFILES_COUNT][LOGO_SIZE];

EncoderConf encoderConfig[ENCODERS_COUNT][PROFILES_COUNT]; // Create the encoder config with 3 encoders
KeyConf keyConf[KEYS_COUNT][PROFILES_COUNT];               // Create the key config with 6 keys
RGBConf rgbConf[PROFILES_COUNT];                           // Create the rgb conf foreach profile

int encodersPosition[3] = {50, 50, 50};  // temp virtual position of encoders
int encodersLastValue[3] = {50, 50, 50}; // Value of encoders

const byte keysPins[6] = {key0Pin, key1Pin, key2Pin, key3Pin, key4Pin, key5Pin};                                                                                   // Pins of all keys
const byte encodersPins[9] = {encoderA0Pin, encoderB0Pin, encoderKey0Pin, encoderA1Pin, encoderB1Pin, encoderKey1Pin, encoderA2Pin, encoderB2Pin, encoderKey2Pin}; // Pins of all encodes

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // config the display

bool keyPressed[KEYS_COUNT];         // current state of key
bool encoderPressed[ENCODERS_COUNT]; // current state of encoder key
bool selectProfileMode = false;      // If is true, all function are disabled, and keys do the profile selection

// ---- Millis for timer ----
unsigned long encoderMillis;
unsigned long selectProfileMillis; // to blink the profile number

byte currentProfile = 0; // Current selected profile is 0 by default

const byte repeatDelay = 5;  //
unsigned long currentMillis; // ----------------------------a renomer
unsigned long tickTime;      // for the scrolling text

String getArgs(String data, char separator, short index)
{
  // This Function separate String with special characters
  short found = 0;
  short strIndex[] = {0, -1};
  short maxIndex = data.length() - 1;

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

  byte pin;
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

void saveToEEPROM() // Save all config into the atmega eeprom
{

#ifdef VERBOSE
  Serial.println("-----Save to EEPROM-----");
#endif

  short eepromAddress = 0;
  for (byte profile = 0; profile <= 5; profile++)
  {
#ifdef VERBOSE
    String txt = "---- Profile " + String(profile) + " ----";
    Serial.println(txt);
#endif
    for (byte i = 0; i < 6; i++)
    {
#ifdef VERBOSE
      Serial.print("Key");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(keyConf[i][currentProfile].mode);
      Serial.print(":");
      Serial.print(keyConf[i][currentProfile].value[0]);
      Serial.print(",");
      Serial.print(keyConf[i][currentProfile].value[1]);
      Serial.print(",");
      Serial.println(keyConf[i][currentProfile].value[2]);
#endif
      // Key mode
      EEPROM.put(eepromAddress, keyConf[i][currentProfile].mode);
      eepromAddress += sizeof(keyConf[i][currentProfile].mode);

      for (byte nValue = 0; nValue <= 2; nValue++)
      {
        EEPROM.put(eepromAddress, keyConf[i][currentProfile].value[nValue]);
        eepromAddress += sizeof(keyConf[i][currentProfile].value[nValue]);
      }
    }

    for (byte i = 0; i < 3; i++)
    {
#ifdef VERBOSE
      Serial.print("Encoder");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(encoderConfig[i][currentProfile].mode);
      Serial.print(":");
      Serial.print(encoderConfig[i][currentProfile].value[0]);
      Serial.print(",");
      Serial.print(encoderConfig[i][currentProfile].value[1]);
      Serial.print(",");
      Serial.println(encoderConfig[i][currentProfile].value[2]);
#endif

      // Save Mode
      EEPROM.put(eepromAddress, encoderConfig[i][currentProfile].mode);
      eepromAddress += sizeof(encoderConfig[i][currentProfile].mode);

      for (byte nValue = 0; nValue <= 2; nValue++)
      {
        EEPROM.put(eepromAddress, encoderConfig[i][currentProfile].value[nValue]);
        eepromAddress += sizeof(encoderConfig[i][currentProfile].value[nValue]);
      }
    }
  }
  
#ifdef VERBOSE
  Serial.print("EEPROM size: ");
  Serial.println(eepromAddress);
#endif
}

void readEEPROM() // Read all config from the atmega eeprom and store it into the temp config
{
#ifdef VERBOSE
  Serial.println("-----Read from EEPROM-----");
#endif
  short eepromAddress = 0;
  for (byte profile = 0; profile <= 5; profile++)
  {
#ifdef VERBOSE
    String txt = "---- Profile " + String(profile) + " ----";
    Serial.println(txt);
#endif
    for (byte i = 0; i < 6; i++) // foreach keys
    {
#ifdef VERBOSE
      Serial.print("Key");
      Serial.print(i);
      Serial.print(" ");

      Serial.print(keyConf[i][profile].mode);
      Serial.print(":");
      Serial.print(keyConf[i][profile].value[0]);
      Serial.print(",");
      Serial.print(keyConf[i][profile].value[1]);
      Serial.print(",");
      Serial.println(keyConf[i][profile].value[2]);
#endif
      // Get mode
      EEPROM.get(eepromAddress, keyConf[i][profile].mode);
      eepromAddress += sizeof(keyConf[i][profile].mode);

      for (byte nValue = 0; nValue <= 2; nValue++)
      {
        EEPROM.get(eepromAddress, keyConf[i][profile].value[nValue]);
        eepromAddress += sizeof(keyConf[i][profile].value[nValue]);
      }
    }
    for (byte i = 0; i < 3; i++) // foreach encoder
    {
#ifdef VERBOSE
      Serial.print("Encoder");
      Serial.print(i);
      Serial.print(" ");
      Serial.print(encoderConfig[i][profile].mode);
      Serial.print(":");
      Serial.print(encoderConfig[i][profile].value[0]);
      Serial.print(":");
      Serial.print(encoderConfig[i][profile].value[1]);
      Serial.print(":");
      Serial.println(encoderConfig[i][profile].value[3]);

#endif

      // Get Mode
      EEPROM.get(eepromAddress, encoderConfig[i][profile].mode);
      eepromAddress += sizeof(encoderConfig[i][profile].mode);

      for (byte nValue = 0; nValue <= 2; nValue++)
      {
        EEPROM.get(eepromAddress, encoderConfig[i][profile].value[nValue]);
        eepromAddress += sizeof(encoderConfig[i][profile].value[nValue]);
      }
    }
  }
}

void configFont()
{
  oled.setFont(&FreeSans12pt7b);    // set the font
  oled.setTextSize(1);              // set the font size to 1
  oled.setTextColor(SSD1306_WHITE); // Write text in white
}

void scrollText() // function to scoll the text into the display
{
  if (tickTime <= millis()) // every 30 ms
  {
    tickTime = millis() + 30;

    if (textScrolling)
    {
      short textRtn = -12 * textOnDisplay.length();
      if (textX > textRtn)
        textX -= 1;
      else
        textX = -textRtn;

      oled.clearDisplay();           // clear the display
      oled.setTextWrap(false);       // disable text warp on the right border
      oled.setFont(&FreeSans12pt7b); // set the default font
      oled.setTextSize(1);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(textX, ((SCREEN_HEIGHT / 2) + FONT_HEIGHT)); // Center the text
      oled.println(textOnDisplay);

      oled.display(); // print the text
    }
  }
}

void centerText(String txt) // Display the text in the center of the screen
{
  oled.clearDisplay(); // clear the display
  short x1;
  short y1;
  short textW;
  short textH;

  configFont();

  oled.getTextBounds(txt, 0, 0, &x1, &y1, &textW, &textH);                 // get the size of the text and store it into textH and textW
  oled.setCursor((SCREEN_WIDTH - textW) / 2, (SCREEN_HEIGHT + textH) / 2); // set the cursor in the good position

  oled.println(txt); // print the text
  oled.display();    // send to screen
  textScrolling = false;
}

void displayOnScreen(String txt) // display test on screen, check if is center text or scrolling text
{
  short x1;
  short y1;
  short textW;
  short textH;
  oled.setFont(&FreeSans12pt7b);
  oled.getTextBounds(txt, 0, 0, &x1, &y1, &textW, &textH); // get the size of the text and store it into textH and textW

  if (textW > SCREEN_WIDTH - 5) // if the text width is larger than the screen, the text is scrolling
  {
    textScrolling = true;
    textOnDisplay = txt;
    textX = 0;
    tickTime = millis();
  }
  else
  {
    centerText(txt);
  }
}

void setRGB(byte r, byte g, byte b) // Set rgb value for LEDs
{
  // ---- Turn On LEDs with the analog value ----
  analogWrite(ledR, r);
  analogWrite(ledG, g);
  analogWrite(ledB, b);
  // ---- Save Value to the conf ----
  rgbConf[currentProfile].r = r;
  rgbConf[currentProfile].g = g;
  rgbConf[currentProfile].b = b;
}
bool profileBlinkState = true;

void printCurrentProfile()
{
  oled.clearDisplay();
  oled.setCursor(31, 25);
  oled.println(F("Profil"));
  oled.setCursor(90, 25);
  oled.println(String((currentProfile + 1)));
  oled.display();
}

void setProfile(byte profile)
{
  currentProfile = profile;
  selectProfileMode = false;
  String txt = "Profil " + String((currentProfile + 1));
  displayOnScreen(txt);
  setRGB(rgbConf[currentProfile].r, rgbConf[currentProfile].g, rgbConf[currentProfile].b);

  //oled.drawRoundRect(0, 3, LOGO_HEIGHT + 2, LOGO_HEIGHT + 2, 5, WHITE);
  oled.drawRamBitmap(1, 4, LOGO_HEIGHT, LOGO_WIDTH, WHITE, logoConf[currentProfile], 72);

  oled.display();
}

void clearProfileNumber()
{
  oled.writeFillRect(90, 9, 12, 18, BLACK);
}

void selectProfile()
{ // When select profile mode is active
  static String strProfile;
  // Profile Number blink:
  if (selectProfileMillis <= millis())
  { // wait 500ms
    oled.setCursor(90, 25);
    selectProfileMillis = millis() + 500;
    clearProfileNumber();
    if (profileBlinkState)
    {
      profileBlinkState = false;
      oled.print(strProfile);
    }
    else
    {
      profileBlinkState = true;
    }
    oled.display();
  }

  for (byte i = 0; i <= 5; i++) // foreach key
  {
    if (digitalRead(keysPins[i]))
    {
      setProfile(i);
    }
  }

  if (encodersPosition[1] != encodersLastValue[1])
  {
    if (encodersPosition[1] < encodersLastValue[1])
    { // clockWise
      if (currentProfile >= 5)
        currentProfile = 0;
      else
        currentProfile++;
    }
    else
    {
      if (currentProfile <= 0)
        currentProfile = 5;
      else
        currentProfile--;
    }
    strProfile = String((currentProfile + 1));
    oled.setCursor(90, 25);
    clearProfileNumber();
    oled.print(strProfile);

    encodersLastValue[1] = encodersPosition[1];
    selectProfileMillis = millis() + 500;
    oled.display();
  }
  if (!digitalRead(encoderKey1Pin))
  {
    setProfile(currentProfile);
  }
}

void setup()
{
  Serial.begin(9600);
  // Serial.setTimeout(500);
#ifdef VERBOSE
  Serial.println("Start");
#endif

  setRGB(255, 0, 0);

#ifdef VERBOSE
  Serial.println("PinMode:");
#endif
  // Keys
  for (byte i = 0; i < 6; i++)
  {
    pinMode(keysPins[i], INPUT);
#ifdef VERBOSE
    Serial.print("Key:");
    Serial.println(keysPins[i]);
#endif
  }
  // Encoders
  for (byte i = 0; i < 3; i++)
  {
    pinMode(encodersPins[i], INPUT);
#ifdef VERBOSE
    Serial.print("Encoder:");
    Serial.println(encodersPins[i]);
#endif
  }
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  delay(500);

  setRGB(0, 255, 0);

#ifdef VERBOSE
  Serial.println("Setup Interrupts");
#endif
  //------------Interrupts----------
  //  attachInterrupt(digitalPinToInterrupt(encoderA0Pin), encoder0, LOW);
  // attachInterrupt(digitalPinToInterrupt(encoderA1Pin), encoder1, LOW);
  // attachInterrupt(digitalPinToInterrupt(encoderA2Pin), encoder2, LOW);

#ifdef VERBOSE
  Serial.println("Setup HID");
#endif
  Consumer.begin(); // Start HID

#ifdef VERBOSE
  Serial.println("Setup Display");
#endif

  Wire.begin();
  // oled.begin(&Adafruit128x32, 0x3C);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.display();

#ifdef VERBOSE
  Serial.println("Read EEPROM");
#endif

  readEEPROM();

  delay(500);
  setRGB(255, 0, 0);

  displayOnScreen(F("Macropad "));
  delay(500);
  setRGB(0, 255, 0);

  // textOnDisplay.reserve(100);
  strIcon.reserve(300);
}

bool encoderAState[3];

void loop()
{

  for (byte i = 0; i < 3; i++) // forech encoder, check if thre is a nex position and change call the fonction
  {
    if (digitalRead(encodersPins[i * 3]) == 0)
    {
      if (encoderAState[i])
      {
        // alredy press
      }
      else
      {
        // first press
        encoder(i);
        encoderAState[i] = true;
      }
    }
    else
    {
      encoderAState[i] = false;
    }
  }

  if (selectProfileMode) // when the select profile mode is enable
  {
    selectProfile();
  }
  else // Default mode
  {
    // Foreach Keys check the state, and do the action if yes
    for (byte i = 0; i < 6; i++)
    {
      if (digitalRead(keysPins[i]))
      {
        if (keyPressed[i])
        { // if the key is already pressed
        }
        else
        {
          Serial.println("Key" + String(i));
          if (keyConf[i][currentProfile].mode == 0) // System Action
          {
            Consumer.write(ConsumerKeycode(keyConf[i][currentProfile].value[0]));
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
#ifdef VERBOSE
                Serial.print("Press ");
                Serial.println(keyConf[i][currentProfile].value[j]);
#endif
              }
              else if (keyConf[i][currentProfile].value[j] <= 90 && keyConf[i][currentProfile].value[j] >= 65) // alaphabet
              {                                                                                                // ascii normal code exept '<' (60)
                Keyboard.press(keyConf[i][currentProfile].value[j] + 32);                                      // Normal character + 32 (ascii Upercase to lowcase)
#ifdef VERBOSE
                Serial.print("Press ");
                Serial.println(keyConf[i][currentProfile].value[j] + 32);
#endif
              }
              else if (keyConf[i][currentProfile].value[j] <= 57 && keyConf[i][currentProfile].value[j] >= 48) // numbers
              {
                Keyboard.press(keyConf[i][currentProfile].value[j]); // Normal number
              }
              else
              {
                Keyboard.press(KeyboardKeycode(keyConf[i][currentProfile].value[j] - 100)); // Improved keyboard
#ifdef VERBOSE
                Serial.print("Press ");
                Serial.println(keyConf[i][currentProfile].value[j] - 100);
#endif
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
#ifdef VERBOSE
          Serial.print("Encoder" + String(i) + ":UP");
#endif
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
#ifdef VERBOSE
          Serial.print("Encoder" + String(i) + ":DOWN");
#endif
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

    if (!digitalRead(encoderKey1Pin)) // When encoder 1 key is pressed
    {

      if (encoderPressed[1])
      { // if the encoder is already pressed
        // wait
        // Serial.println("Already press");
        if (encoderMillis <= millis() && selectProfileMode == false)
        {
          // 2 second after the begin of the press
          // Serial.println("2S --> profile set");
          // set profile menu
          selectProfileMode = true;
          printCurrentProfile();
          oled.drawBitmap(0, 10, profile_left, PROFILE_H, PROFILE_W, WHITE);
          oled.drawBitmap(119, 10, profile_right, PROFILE_H, PROFILE_W, WHITE);
          oled.display();
          selectProfileMillis = millis();
          while (!digitalRead(encoderKey1Pin))
          {
            /* code */
          }
        }
      }
      else
      { // first press of the encoder
        // Serial.println("First press");
        encoderPressed[1] = true;
        encoderMillis = millis() + 1500;
      }
    }

    if (digitalRead(encoderKey1Pin)) // When encoder 1 key is relese
    {
      // Do the action
      encoderPressed[1] = false;
    }
  }

  if (!digitalRead(encoderKey0Pin) && !digitalRead(encoderKey1Pin) && !digitalRead(encoderKey2Pin)) // install the software
  {
    Consumer.write(ConsumerKeycode(0x223)); // open default web Browser
    delay(1000);
    Keyboard.press(KEY_LEFT_CTRL); // focus the url with CTRL + L
    Keyboard.press(KEY_L);
    delay(10);
    Keyboard.releaseAll();
    delay(100);
    Keyboard.print(F("https://github.com/xmow49/MacroPad-Software/releases")); // enter the url
    delay(100);
    Keyboard.write(KEY_ENTER); // go to the url
  }

  if (Serial.available() > 0)
  {

    String serialMsg; // Store the tmp serial message
    // serialMsg.reserve(200);
    serialMsg = Serial.readString();
    serialMsg.trim();

    if (waitSerialIcon)
    {
      FREERAM_PRINT;
      strIcon = strIcon + serialMsg;
      Serial.println(strIcon);
      unsigned char icon[72];
      strIcon.toCharArray(icon, 72);
      strIcon = "";
      for (byte i = 0; i <= 72; i++)
      {
        logoConf[currentProfile][i] = icon[i];
        Serial.println(icon[i]);
      }
      setProfile(currentProfile);

      // if (msgCount <= 3)
      // {

      //   msgCount++;
      // }

      // else
      // {
      //   // waitSerialIcon = false;
      // }
    }
    else
    {

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
      }

      else if (command == "set-encoder") // Set encoder action
      {
        encoderConfig[arg[0]][currentProfile].mode = arg[1]; // Get the mode and save it in the config
        for (byte i = 0; i < 3; i++)                         // for all values, save it in the config
        {
          encoderConfig[arg[0]][currentProfile].value[i] = arg[i + 2];
        }
      }
      else if (command == "set-text")
      {
        String txt = getArgs(serialMsg, '"', 1);
        txt.remove(txt.length() - 1);
        displayOnScreen(txt);
      }

      else if (command == "get-config") // Get config command
      {
        for (byte profile = 0; profile <= 5; profile++)
        {
#ifdef VERBOSE
          String txt = "---- Profile " + String(profile) + " ----";
          Serial.println(txt);
#endif
          for (byte i = 0; i < 6; i++) // for all keys, get the config and display it
          {
            // Serial.print("Key");
            // Serial.print(i);
            // Serial.print(" ");
            // Serial.print(keyConf[i][profile].mode);
            // Serial.print(" ");
            // Serial.print(keyConf[i][profile].value[0]);
            // Serial.print(":");
            // Serial.print(keyConf[i][profile].value[1]);
            // Serial.print(":");
            // Serial.println(keyConf[i][profile].value[2]);
          }
          for (byte i = 0; i < 3; i++) // for all encoders, get the config and display it
          {
            // Serial.print("Encoder");
            // Serial.print(i);
            // Serial.print(" ");
            // Serial.print(encoderConfig[i][profile].mode);
            // Serial.print(" ");
            // Serial.print(encoderConfig[i][profile].value[0]);
            // Serial.print(":");
            // Serial.print(encoderConfig[i][profile].value[1]);
            // Serial.print(":");
            // Serial.println(encoderConfig[i][profile].value[2]);
          }
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

      else if (command == "set-rgb") // set-rgb <Red: 0 - 255> <Green: 0 - 255> <Blue: 0 - 255>
      {
        setRGB(arg[0], arg[1], arg[2]); // Set RGB LED color to rgb value from the command :
      }
      else if (command == "reset-config") // reset all the config to 0
      {
        for (byte profile = 0; profile <= 5; profile++)
        {
          String txt = "Profile " + String(profile) + " :";
          Serial.println(txt);
          for (byte i = 0; i < 3; i++)
          {
            encoderConfig[i][profile].mode = 0;
            for (byte j = 0; j < 3; j++)
            {
              encoderConfig[i][profile].value[j] = 0;
            }
          }

          for (byte i = 0; i < 6; i++)
          {
            keyConf[i][profile].mode = 0;
            for (byte j = 0; j < 3; j++)
            {
              keyConf[i][profile].value[j] = 0;
            }
          }
        }
      }
      else if (command == "set-icon")
      {
        waitSerialIcon = true;
        msgCount = 0;
        strIcon = "";
      }
      else
      {
        Serial.print(command);
        Serial.println(": error");
      }
      command = "";
    }
    Serial.println("OK");
    // serialMsg = "";
  }

  scrollText();
}
