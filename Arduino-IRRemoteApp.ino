#include <NewTone.h>
#include <IRremote.h>

String _inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
IRsend _sender;
unsigned int _buff[200];
int _freq = 38;

// Used by Tone library for enable interrupts
extern uint8_t _pinMask;         // Pin bitmask.

// Pin for PWM tone
const int TONE_PIN = 10;

// Wire the IR receiver to pin 9!
int RECV_PIN = 9;
IRrecv irrecv(RECV_PIN);
decode_results results;

int RemoteCodes[] = {20737, 62593, 2945, 44801, 51329, 27649, 33537, 9857, 22401, 64257};
int NoteFreqs[] = {262, 294, 330, 349, 392, 440, 494, 523, 587, 659};

void setup()
{
  Serial.begin(9600);
  // reserve bytes for the inputString:
  _inputString.reserve(400);
}

void loop() {

  if (stringComplete) {
    int i=0;
    String strCommand;
    
    GetCommand(_inputString, i, strCommand);

    if(strCommand.equals("send"))
      onSendIrCommand(_inputString, i);
    else if(strCommand.equals("test"))
      onTestCommand(_inputString, i);
    else if(strCommand.equals("r"))
      onReceiveCommand(_inputString, i);
    else if(strCommand.equals("s"))
      onSendLastCommand(_inputString, i);
    else if(strCommand.equals("sendrc6"))
      onSendRc6Command(_inputString, i);
    else if(strCommand.equals("freq"))
      onSetFreqCommand(_inputString, i);
    else if(strCommand.equals("tone"))
      onToneCommand(_inputString, i);
    else if(strCommand.equals("toneramp"))
      onToneRCommand(_inputString, i);
    
    // clear the string:
    _inputString = "";
    stringComplete = false;
  }
}

// http://www.partow.net/programming/hashfunctions/#RSHashFunction
unsigned int RSHash(unsigned int *pArray, int len)
{
   unsigned int b    = 378551;
   unsigned int a    = 63689;
   unsigned int hash = 0;
   unsigned int i    = 0;
   unsigned int *pData = pArray + 3;
   
   unsigned int minVal = 1000000;
   
   // Get the minimum value but exclude zero.
   for(i = 3; i < len; pData++, i++)
   {
     unsigned int value = *pData;
     if(value < minVal && value > 0)
     {
       minVal = value;
     }
   }

   pData = pArray + 3;
   for(i = 3; i < len; pData++, i++)
   {
      hash = hash * a + (*pData) / minVal;
      a    = a * b;
   }

   return hash;
}

// Override NewTone, don't return until NewTone has finished tone!
void NewToneBlocking(uint8_t pin, unsigned long frequency, unsigned long length) {
  NewTone(pin, frequency, length);
  while(_pinMask > 0)
  {
    delay(0);
  }
}

// Begin receiving from the IR LED
void onReceiveCommand(const String& inputString, int index)
{
  String strArg;
  char szText[100];
  GetArgs(inputString, index, strArg);

  Serial.println("receiving;");
  irrecv.enableIRIn(); // Start the receiver
  
  while(!Serial.available())
  {
    if(irrecv.decode(&results)) {
      if(strArg == "h") {
          if(results.rawlen > 5) {
            Serial.println("hash: " + String(RSHash((unsigned int*)results.rawbuf, results.rawlen)) + ";");
          }
          irrecv.enableIRIn();
      }
      else if(strArg == "i") {
          if(results.rawlen > 5) {
            int hash = RSHash((unsigned int*)results.rawbuf, results.rawlen);
            for(int index=0;index < 10;++index) {
              if(RemoteCodes[index] == hash) {
                Serial.println("Got Key: " + String(index) + ";");
                NewTone(TONE_PIN, NoteFreqs[index], 200);
                break;
              }
            }
            
            //Serial.println("hash: " + String(RSHash((unsigned int*)results.rawbuf, results.rawlen)) + ";");
          }
          irrecv.enableIRIn();
      }
      else {
        dumpArrayAsHex((unsigned int*)results.rawbuf, results.rawlen);
        if(strArg == "t") {
            irrecv.enableIRIn();
        }
        else {
          return;
        }
      }
    }
  }
}

void onSendRc6Command(const String& inputString, int index)
{
  String strArg;
  char szText[100];
  
  GetArgs(inputString, index, strArg);
  unsigned long val = strArg.toInt();
  _sender.sendRC6(val, 24);
  
  sprintf(szText, "Sent RC6: intval = %ld",val);
  Serial.println(szText);
}

void onSendLastCommand(const String& inputString, int index)
{
  char szText[100];
  for(int j=0;j<results.rawlen-1;j++)
  {
    _buff[j] = results.rawbuf[j+1] * 50;
  }
  
  _sender.sendRaw(_buff, results.rawlen-1, _freq);
  sprintf(szText, "Sent %d items, freq = %d;", results.rawlen-1, _freq);
  Serial.println(szText); 
}

void onSetFreqCommand(const String& inputString, int index)
{
  Serial.println("Set freq;");
  char szText[50];
  String strVal;
  GetNextArg(inputString, index, strVal);
  if(strVal.length() > 0)
  {
    _freq = strVal.toInt();
    sprintf(szText, "New freq: %d;", _freq);
    Serial.println(szText);
  }
}

void onToneCommand(const String& inputString, int index)
{
  Serial.println("onToneCommand;");
  String strFreq;
  String strDuration;
  
  GetNextArg(inputString, index, strFreq);
  Serial.println("Freq: " + strFreq);
  GetNextArg(inputString, index, strDuration);
  Serial.println("Duration: " + strDuration);
  
  unsigned long val = strFreq.toInt();
  unsigned long duration = 10000;
  if(strDuration.length() > 0)
  {
    duration = strDuration.toInt();
  }

  NewTone(TONE_PIN, val, duration);
  Serial.println("Tone Completed;");
}

void onToneRCommand(const String& inputString, int index)
{
  Serial.println("onToneRCommand;");
  String strArg;
  int startFreq, endFreq, freqStep, duration, reps;
  int intargs[5];
  
  for(int i=0;i<5;++i)
  {
    GetNextArg(inputString, index, strArg);
    if(strArg.length() == 0)
    {
      Serial.println("onToneRCommand: Error at arg " + i);
      return;
    }
    intargs[i] = strArg.toInt();
  }
  
  ToneRamp(intargs[0], intargs[1], intargs[2], intargs[3], intargs[4]);  

  Serial.println("Tone Completed;");
}

void ToneRamp(int startFreq, int endFreq, int freqStep, int duration, int reps)
{
  for(int rep=0;rep<reps;++rep)
  {
    if(startFreq < endFreq)
    {
      for(int freq=startFreq;freq<endFreq;freq += freqStep)
      {
        NewToneBlocking(TONE_PIN, freq, duration);
      }
    }
    else
    {
      for(int freq=startFreq;freq>endFreq;freq -= freqStep)
      {
        NewToneBlocking(TONE_PIN, freq, duration);
      }
    }  
  }
}

void onTestCommand(const String &inputString, int index)
{
  Serial.println("Testing;");
}

void onSendIrCommand(const String& inputString, int index)
{
  char szText[50];
  int buffCount = 0;
  
  // Convert the hex dump to binary, placing it in the buffer.
  hexDumpToArray(inputString, index, _buff, buffCount);
  
  // Send the pulses
  _sender.sendRaw(_buff, buffCount, _freq);
  sprintf(szText, "Sent: %d items, freq = %d;", buffCount, _freq);
  Serial.println(szText); 
}

// Get the next argument in the input string, increasing the index
void GetNextArg(const String &inputString, int &i, String &arg)
{
  if(i >= inputString.length())
  {
     arg = String("");
     return;
  }
  
  int iNext = inputString.indexOf(',', i);
  if(iNext >= 0)
  {
    arg = inputString.substring(i, iNext);
    i = iNext+1;
  }
  else
  {
    arg = inputString.substring(i, inputString.length());
    i += arg.length();
  }
}

void GetArgs(const String &inputString, int i, String &args)
{
  if(i < inputString.length())
  {
    args = inputString.substring(i);
  }
  else
  {
    args = String("");
  }
}

void GetCommand(String &inputString, int &i, String &command)
{
  if(i < inputString.length())
  {
    int iNext = inputString.indexOf(':', i);
    if(iNext >= 0)
    {
      command = inputString.substring(i, iNext);
      i = iNext+1;
    }
  }
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 

    if (inChar == ';') {
      stringComplete = true;
    }
    else {
      // add it to the inputString:
      _inputString += inChar;
    } 
  }
}

void dumpArrayAsHex(unsigned int *pArray, int len)
{
  char szText[10];
  Serial.print("dump: ");
  for(int i=0;i<len;++i)
  {
    if(i == 0)
    {
      sprintf(szText, "%X", pArray[i]);
    }
    else
    {
      sprintf(szText, ", %X", pArray[i]);
    }
    
    Serial.print(szText);
  }
  Serial.println(";");
}

void hexDumpToArray(const String& inputString, int index, unsigned int* pBuffer, int& count)
{
  char szText[50];
  count = 0;
  
  while(index < inputString.length())
  {
    unsigned int val;
    String strVal; 
    int iNext = inputString.indexOf(',', index);
    if(iNext >= 0)
    {
      strVal = inputString.substring(index, iNext);
      index = iNext+1;
    }
    else
    {
      strVal = inputString.substring(index);
      index += strVal.length();
    }
    
    strVal.toCharArray(szText, 10);
    sscanf(szText, "%x", &val);

    pBuffer[count] = val * 50;
    count++;
  }
}


