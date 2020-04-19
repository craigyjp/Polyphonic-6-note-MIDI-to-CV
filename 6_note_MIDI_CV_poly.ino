/* 
 *    MIDI2CV_Poly
 *    Copyright (C) 2020 Craig Barnes
 *    
 *    A big thankyou to Elkayem for his midi to cv code
 *    A big thankyou to ElectroTechnique for his polyphonic tsynth that I used for the poly notes routine
 *  
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License <http://www.gnu.org/licenses/> for more details.
 */
 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <MIDI.h>

// OLED I2C is used on pins 18 and 19 for Teensy 3.x

// Voices available
#define  NO_OF_VOICES 6

//Note DACS
#define DAC_NOTE1      7
#define DAC_NOTE2      8
#define DAC_NOTE3      9
#define DAC_NOTE4      10
#define DAC_NOTE5      24
#define DAC_NOTE6      25

// Pitchbend and CC DAC
#define DAC7      26

//Trig outputs
#define TRIG_NOTE1  27
#define TRIG_NOTE2  28
#define TRIG_NOTE3  29
#define TRIG_NOTE4  30
#define TRIG_NOTE5  31
#define TRIG_NOTE6  32

//Gate outputs
#define GATE_NOTE1  33
#define GATE_NOTE2  34
#define GATE_NOTE3  35
#define GATE_NOTE4  36
#define GATE_NOTE5  37
#define GATE_NOTE6  38

#define ENC_A   14
#define ENC_B   15
#define ENC_BTN 16

#define GATE(CH) (CH==0 ? GATE_NOTE1 : (CH==1 ? GATE_NOTE2 : (CH==2 ? GATE_NOTE3: (CH==3 ? GATE_NOTE4: (CH==4 ? GATE_NOTE5 : (CH==5 ? GATE_NOTE6 : GATE_NOTE6))))))
#define TRIG(CH) (CH==0 ? TRIG_NOTE1 : (CH==1 ? TRIG_NOTE2 : (CH==2 ? TRIG_NOTE3: (CH==3 ? TRIG_NOTE4: (CH==4 ? TRIG_NOTE5 : (CH==5 ? TRIG_NOTE6 : TRIG_NOTE6))))))

#define NOTE_DAC(CH) (CH==0 ? DAC_NOTE1 : (CH==1 ? DAC_NOTE2 : (CH==2 ? DAC_NOTE3: (CH==3 ? DAC_NOTE4: (CH==4 ? DAC_NOTE5 : (CH==5 ? DAC_NOTE6 : DAC_NOTE6))))))
#define NOTE_AB(CH)  (CH==1 ? 1 : 0)

#define VEL_DAC(CH) (CH==0 ? DAC_NOTE1 : (CH==1 ? DAC_NOTE2 : (CH==2 ? DAC_NOTE3: (CH==3 ? DAC_NOTE4: (CH==4 ? DAC_NOTE5 : (CH==5 ? DAC_NOTE6 : DAC_NOTE6))))))
#define VEL_AB(CH)  (CH==1 ? 0 : 1)

#define PITCH_DAC DAC7
#define PITCH_AB  0
#define CC_DAC    DAC7
#define CC_AB     1

// Scale Factor
#define NOTE_SF 47.069f 

#define OLED_RESET 17
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int encoderPos, encoderPosPrev;
Bounce encButton = Bounce(); 

enum Menu {
  SETTINGS,
  KEYBOARD_MODE_SET_CH,
  SCALE_FACTOR_SET_CH,
  MIDI_CHANNEL_SET_CH
} menu;

int keyboardMode = 0;
char gateTrig[] = "TTTTTT";
float sfAdj;

uint8_t pitchBendChan;
uint8_t ccChan;
int masterChan;
int8_t d2, i;

unsigned long trigTimer[6] = {0};

struct VoiceAndNote {
  int note;
  int velocity;
  long timeOn;
};

struct VoiceAndNote voices[NO_OF_VOICES] = {
  { -1, -1, 0},
  { -1, -1, 0},
  { -1, -1, 0},
  { -1, -1, 0},
  { -1, -1, 0},
  { -1, -1, 0}
};

boolean voiceOn[NO_OF_VOICES] = {false, false, false, false, false, false};
int voiceToReturn = -1;//Initialise to 'null'
long earliestTime = millis();//For voice allocation - initialise to now
int  prevNote = 0;//Initialised to middle value
int vcoOctaveA = 0;
int vcoOctaveB = 0;
int unison = 0;

// MIDI setup
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
const int channel = 1;
 
// EEPROM Addresses
#define ADDR_KEYBOARD_MODE 0
#define ADDR_GATE_TRIG     6
#define ADDR_PITCH_BEND    12
#define ADDR_CC            13
#define ADDR_SF_ADJUST     14
#define ADDR_MASTER_CHAN   20

bool highlightEnabled = false;  // Flag indicating whether highighting should be enabled on menu
#define HIGHLIGHT_TIMEOUT 20000  // Highlight disappears after 20 seconds.  Timer resets whenever encoder turned or button pushed
unsigned long int highlightTimer = 0;  

void setup()
{
 pinMode(GATE_NOTE1, OUTPUT);
 pinMode(GATE_NOTE2, OUTPUT);
 pinMode(GATE_NOTE3, OUTPUT);
 pinMode(GATE_NOTE4, OUTPUT);
 pinMode(GATE_NOTE5, OUTPUT);
 pinMode(GATE_NOTE6, OUTPUT);
 pinMode(TRIG_NOTE1, OUTPUT);
 pinMode(TRIG_NOTE2, OUTPUT);
 pinMode(TRIG_NOTE3, OUTPUT);
 pinMode(TRIG_NOTE4, OUTPUT);
 pinMode(TRIG_NOTE5, OUTPUT);
 pinMode(TRIG_NOTE6, OUTPUT);
 pinMode(DAC_NOTE1, OUTPUT);
 pinMode(DAC_NOTE2, OUTPUT);
 pinMode(DAC_NOTE3, OUTPUT);
 pinMode(DAC_NOTE4, OUTPUT);
 pinMode(DAC_NOTE5, OUTPUT);
 pinMode(DAC_NOTE6, OUTPUT);
 pinMode(DAC7, OUTPUT);
 pinMode(ENC_A, INPUT_PULLUP);
 pinMode(ENC_B, INPUT_PULLUP);
 pinMode(ENC_BTN,INPUT_PULLUP);
  
 digitalWrite(GATE_NOTE1,LOW);
 digitalWrite(GATE_NOTE2,LOW);
 digitalWrite(GATE_NOTE3,LOW);
 digitalWrite(GATE_NOTE4,LOW);
 digitalWrite(GATE_NOTE5,LOW);
 digitalWrite(GATE_NOTE6,LOW);
 digitalWrite(TRIG_NOTE1,LOW);
 digitalWrite(TRIG_NOTE2,LOW);
 digitalWrite(TRIG_NOTE3,LOW);
 digitalWrite(TRIG_NOTE4,LOW);
 digitalWrite(TRIG_NOTE5,LOW);
 digitalWrite(TRIG_NOTE6,LOW);
 digitalWrite(DAC_NOTE1,HIGH);
 digitalWrite(DAC_NOTE2,HIGH);
 digitalWrite(DAC_NOTE3,HIGH);
 digitalWrite(DAC_NOTE4,HIGH);
 digitalWrite(DAC_NOTE5,HIGH);
 digitalWrite(DAC_NOTE6,HIGH);
 digitalWrite(DAC7,HIGH);

 SPI.begin();
 MIDI.begin(masterChan);
 MIDI.setHandleNoteOn(myNoteOn);
 MIDI.setHandleNoteOff(myNoteOff);
 MIDI.setHandlePitchBend(myPitchBend);
 MIDI.setHandleControlChange(myControlChange);
 
 display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // OLED I2C Address, may need to change for different device,
                                            // Check with I2C_Scanner

 // Wire.setClock(100000L);  // Uncomment to slow down I2C speed

 // Read Settings from EEPROM
 for (int i=6; i<1; i++) {
  gateTrig[i] = (char)EEPROM.read(ADDR_GATE_TRIG+i);
  if (gateTrig[i] != 'G' && gateTrig[i] != 'T') gateTrig[i] = 'T';
 }
 for (int i=0; i<1; i++) {
   keyboardMode = (char)EEPROM.read(ADDR_KEYBOARD_MODE);
        if (keyboardMode == 1 ) unison = 1;
        if (keyboardMode == 0 ) unison = 0;
        if (keyboardMode == 2 ) unison = 2;
   EEPROM.get(ADDR_SF_ADJUST, sfAdj);
   masterChan = (int)EEPROM.read(ADDR_MASTER_CHAN);
  // Set defaults if EEPROM not initialized
  if (keyboardMode != 0 && keyboardMode != 1 && keyboardMode != 2) keyboardMode = 0;
  if ((sfAdj < 0.9f) || (sfAdj > 1.1f) || isnan(sfAdj)) sfAdj = 1.0f;
  if (masterChan >15) masterChan = 0;
 }
  
 pitchBendChan = EEPROM.read(ADDR_MASTER_CHAN);
 ccChan = EEPROM.read(ADDR_MASTER_CHAN);

// Set defaults if EEPROM not initialized
 if (pitchBendChan > 15) pitchBendChan = 0;
 if (ccChan > 15) ccChan = 0;

 encButton.attach(ENC_BTN);
 encButton.interval(25);  // interval in ms

 menu = SETTINGS;
 updateSelection();
}

void myPitchBend(byte channel, int bend){
  if (MIDI.getChannel() == pitchBendChan) {
          // Pitch bend output from 0 to 1023 mV.  Left shift d2 by 4 to scale from 0 to 2047.
          // With DAC gain = 1X, this will yield a range from 0 to 1023 mV.  Additional amplification
          // after DAC will rescale to -1 to +1V.
          d2 = MIDI.getData2(); // d2 from 0 to 127, mid point = 64
          setVoltage(PITCH_DAC, PITCH_AB, 0, d2<<4);  // DAC7, channel 0, gain = 1X
        }
}

void myControlChange(byte channel, byte number, byte value){
 if (MIDI.getChannel() == ccChan) {
          d2 = MIDI.getData2(); 
          // CC range from 0 to 4095 mV  Left shift d2 by 5 to scale from 0 to 4095, 
          // and choose gain = 2X
          setVoltage(CC_DAC, CC_AB, 1, d2<<5);  // DAC7, channel 1, gain = 2X
        } 
}

void myNoteOn(byte channel, byte note, byte velocity) {
  //Check for out of range notes
  if (note< 0 || note> 127) return;

  prevNote = note;
  if (unison == 0) {
    switch (getVoiceNo(-1)) {
      case 1:
        voices[0].note = note;
        voices[0].velocity = velocity;
        voices[0].timeOn = millis();
        updateVoice1();
        digitalWrite(GATE_NOTE1,HIGH);
        digitalWrite(TRIG_NOTE1,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE1,LOW);
        voiceOn[0] = true;
        break;
      case 2:
        voices[1].note = note;
        voices[1].velocity = velocity;
        voices[1].timeOn = millis();
        updateVoice2();
        digitalWrite(GATE_NOTE2,HIGH);
        digitalWrite(TRIG_NOTE2,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE2,LOW);
        voiceOn[1] = true;
        break;
      case 3:
        voices[2].note = note;
        voices[2].velocity = velocity;
        voices[2].timeOn = millis();
        updateVoice3();
        digitalWrite(GATE_NOTE3,HIGH);
        digitalWrite(TRIG_NOTE3,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE3,LOW);
        voiceOn[2] = true;
        break;
      case 4:
        voices[3].note = note;
        voices[3].velocity = velocity;
        voices[3].timeOn = millis();
        updateVoice4();
        digitalWrite(GATE_NOTE4,HIGH);
        digitalWrite(TRIG_NOTE4,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE4,LOW);
        voiceOn[3] = true;
        break;
      case 5:
        voices[4].note = note;
        voices[4].velocity = velocity;
        voices[4].timeOn = millis();
        updateVoice5();
        digitalWrite(GATE_NOTE5,HIGH);
        digitalWrite(TRIG_NOTE5,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE5,LOW);
        voiceOn[4] = true;
        break;
      case 6:
        voices[5].note = note;
        voices[5].velocity = velocity;
        voices[5].timeOn = millis();
        updateVoice6();
        digitalWrite(GATE_NOTE6,HIGH);
        digitalWrite(TRIG_NOTE6,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE6,LOW);
        voiceOn[5] = true;
        break;
    }
  } else if (unison == 2)
  // MONO MODE
        { voices[0].note = note;
        voices[0].velocity = velocity;
        voices[0].timeOn = millis();
        updateVoice1();
        digitalWrite(GATE_NOTE1,HIGH);
        digitalWrite(TRIG_NOTE1,HIGH);
        delay(20);
        digitalWrite(TRIG_NOTE1,LOW);
        voiceOn[0] = true;
  } else if (unison == 1)
  {
    //UNISON MODE
    voices[0].note = note;
    voices[0].velocity = velocity;
    voices[0].timeOn = millis();
    updateVoice1();
    voices[1].note = note;
    voices[1].velocity = velocity;
    voices[1].timeOn = millis();
    updateVoice2();
    voices[2].note = note;
    voices[2].velocity = velocity;
    voices[2].timeOn = millis();
    updateVoice3();
    voices[3].note = note;
    voices[3].velocity = velocity;
    voices[3].timeOn = millis();
    updateVoice4();
    voices[4].note = note;
    voices[4].velocity = velocity;
    voices[4].timeOn = millis();
    updateVoice5();
    voices[5].note = note;
    voices[5].velocity = velocity;
    voices[5].timeOn = millis();
    updateVoice6();

 digitalWrite(GATE_NOTE1,HIGH);
 digitalWrite(GATE_NOTE2,HIGH);
 digitalWrite(GATE_NOTE3,HIGH);
 digitalWrite(GATE_NOTE4,HIGH);
 digitalWrite(GATE_NOTE5,HIGH);
 digitalWrite(GATE_NOTE6,HIGH);

    voiceOn[0] = true;
    voiceOn[1] = true;
    voiceOn[2] = true;
    voiceOn[3] = true;
    voiceOn[4] = true;
    voiceOn[5] = true;
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {
  if (unison == 0) {
    switch (getVoiceNo(note)) {
      case 1:
        digitalWrite(GATE_NOTE1,LOW);
        voices[0].note = -1;
        voiceOn[0] = false;
        break;
      case 2:
        digitalWrite(GATE_NOTE2,LOW);
        voices[1].note = -1;
        voiceOn[1] = false;
        break;
      case 3:
        digitalWrite(GATE_NOTE3,LOW);
        voices[2].note = -1;
        voiceOn[2] = false;
        break;
      case 4:
        digitalWrite(GATE_NOTE4,LOW);
        voices[3].note = -1;
        voiceOn[3] = false;
        break;
      case 5:
        digitalWrite(GATE_NOTE5,LOW);
        voices[4].note = -1;
        voiceOn[4] = false;
        break;
      case 6:
        digitalWrite(GATE_NOTE6,LOW);
        voices[5].note = -1;
        voiceOn[5] = false;
        break;
    }
  } else if (unison == 2)
  {
    //MONO MODE
    firstNoteOff();
  } else if (unison == 1)
  {
    //UNISON MODE
    allNotesOff();
  }
}

void allNotesOff() {
 digitalWrite(GATE_NOTE1,LOW);
 digitalWrite(GATE_NOTE2,LOW);
 digitalWrite(GATE_NOTE3,LOW);
 digitalWrite(GATE_NOTE4,LOW);
 digitalWrite(GATE_NOTE5,LOW);
 digitalWrite(GATE_NOTE6,LOW);

  voices[0].note = -1;
  voices[1].note = -1;
  voices[2].note = -1;
  voices[3].note = -1;
  voices[4].note = -1;
  voices[5].note = -1;

  voiceOn[0] = false;
  voiceOn[1] = false;
  voiceOn[2] = false;
  voiceOn[3] = false;
  voiceOn[4] = false;
  voiceOn[5] = false;
}

void firstNoteOff() {
 digitalWrite(GATE_NOTE1,LOW);
  voices[0].note = -1;
  voiceOn[0] = false;
}

int getVoiceNo(int note) {
  voiceToReturn = -1;//Initialise to 'null'
  earliestTime = millis();//Initialise to now
  if (note == -1) {
    //NoteOn() - Get the oldest free voice (recent voices may be still on release stage)
    for (int i = 0; i <  NO_OF_VOICES; i++) {
      if (voices[i].note == -1) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    if (voiceToReturn == -1) {
      //No free voices, need to steal oldest sounding voice
      earliestTime = millis();//Reinitialise
      for (int i = 0; i <  NO_OF_VOICES; i++) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    return voiceToReturn + 1;
  } else {
    //NoteOff() - Get voice number from note
    for (int i = 0; i <  NO_OF_VOICES; i++) {
      if (voices[i].note == note) {
        return i + 1;
      }
    }
  }
  //Shouldn't get here, return voice 1
  return 1;
}

void updateVoice1() {
  if (unison == 1) {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE1, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE1, 1, 1, velmV <<5 );
  }
  else
  {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE1, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE1, 1, 1, velmV <<5 );  
  }
}

void updateVoice2() {
  if (unison == 1) {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE2, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE2, 1, 1, velmV <<5 );
  }
  else
  {
  unsigned int mV = (unsigned int) ((float) voices[1].note * NOTE_SF * sfAdj + 0.5);  
  setVoltage(DAC_NOTE2, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[1].velocity);
  setVoltage(DAC_NOTE2, 1, 1, velmV <<5 );  
  } 
}

void updateVoice3() {
  if (unison == 1) {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE3, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE3, 1, 1, velmV <<5 );
  }
  else
  { 
  unsigned int mV = (unsigned int) ((float) voices[2].note * NOTE_SF * sfAdj + 0.5);  
  setVoltage(DAC_NOTE3, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[2].velocity);
  setVoltage(DAC_NOTE3, 1, 1, velmV <<5 );   
  } 
}

void updateVoice4() {
  if (unison == 1) {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE4, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE4, 1, 1, velmV <<5 );
  }
  else
  {
  unsigned int mV = (unsigned int) ((float) voices[3].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE4, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[3].velocity);
  setVoltage(DAC_NOTE4, 1, 1, velmV <<5 );   
  } 
}

void updateVoice5() {
  if (unison == 1) {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE5, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE5, 1, 1, velmV <<5 );
  }
  else
  {
  unsigned int mV = (unsigned int) ((float) voices[4].note * NOTE_SF * sfAdj + 0.5);
  setVoltage(DAC_NOTE5, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[4].velocity);
  setVoltage(DAC_NOTE5, 1, 1, velmV <<5 );   
  } 
}

void updateVoice6() {
  if (unison == 1) {
  unsigned int mV = (unsigned int) ((float) voices[0].note * NOTE_SF * sfAdj + 0.5); 
  setVoltage(DAC_NOTE6, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[0].velocity);
  setVoltage(DAC_NOTE6, 1, 1, velmV <<5 );
  }
  else
  {
  unsigned int mV = (unsigned int) ((float) voices[1].note * NOTE_SF * sfAdj + 0.5);
  setVoltage(DAC_NOTE6, 0, 1, mV);
  unsigned int velmV = (unsigned int) ((float) voices[5].velocity);
  setVoltage(DAC_NOTE6, 1, 1, velmV <<5 );  
  } 
}

bool notes[6][88] = {0}, initial_loop = 1; 
int8_t noteOrder[6][10] = {0}, orderIndx[6] = {0};

void loop()
{
//  int8_t noteMsg, velocity, channel, i;

int8_t i;

  updateEncoderPos();  
  encButton.update();

 if (encButton.fell()) {  
   if (initial_loop == 1) {
     initial_loop = 0;  // Ignore first push after power-up
    }
   else {
     updateMenu();
    }
   }

  // Check if highlighting timer expired, and remove highlighting if so
  if (highlightEnabled && ((millis() - highlightTimer) > HIGHLIGHT_TIMEOUT)) {  
    highlightEnabled = false;
    menu = SETTINGS;    // Return to top level menu
    updateSelection();  // Refresh screen without selection highlight
  }
  
  for (i=0;i<6;i++) {
    if ((trigTimer[i] > 0) && (millis() - trigTimer[i] > 20) && gateTrig[i] == 'T') { 
      digitalWrite(GATE(i),LOW); // Set trigger low after 20 msec 
      trigTimer[i] = 0;  
    }
  }
MIDI.read(masterChan);//MIDI 5 Pin DIN 
        
}

void setVoltage(int dacpin, bool channel, bool gain, unsigned int mV)
{
  int command = channel ? 0x9000 : 0x1000;

  command |= gain ? 0x0000 : 0x2000;
  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(dacpin,LOW);
  SPI.transfer(command>>8);
  SPI.transfer(command&0xFF);
  digitalWrite(dacpin,HIGH);
  SPI.endTransaction();
}


void updateEncoderPos() {
    static int encoderA, encoderB, encoderA_prev;   

    encoderA = digitalRead(ENC_A); 
    encoderB = digitalRead(ENC_B);
      
    if((!encoderA) && (encoderA_prev)){ // A has gone from high to low 
      if (highlightEnabled) { // Update encoder position
        encoderPosPrev = encoderPos;
        encoderB ? encoderPos++ : encoderPos--;  
      }
      else { 
        highlightEnabled = true;
        encoderPos = 0;  // Reset encoder position if highlight timed out
        encoderPosPrev = 0;
      }
      highlightTimer = millis(); 
      updateSelection();
    }
    encoderA_prev = encoderA;     
}

int setCh;
//char setMode[6];

void updateMenu() {  // Called whenever button is pushed

  if (highlightEnabled) { // Highlight is active, choose selection
    switch (menu) {

      case SETTINGS:
        switch (mod(encoderPos,3)) {
          case 0: 
            menu = KEYBOARD_MODE_SET_CH;
            break;
          case 1: 
            menu = SCALE_FACTOR_SET_CH;
            break;
          case 2:
            menu = MIDI_CHANNEL_SET_CH;
            break;
        }
        break;
        
      case KEYBOARD_MODE_SET_CH:  // Save keyboard mode setting to EEPROM
        menu = SETTINGS;
        EEPROM.write(ADDR_KEYBOARD_MODE, keyboardMode);
        if (keyboardMode == 0 ) unison = 0; // Poly mode
        if (keyboardMode == 1 ) unison = 1; // Unison mode
        if (keyboardMode == 2 ) unison = 2; // Mono Mode
        break; 
        
      case SCALE_FACTOR_SET_CH:  // Save scale factor setting to EEPROM
        menu = SETTINGS;
        EEPROM.write(ADDR_SF_ADJUST, sfAdj);
        break;
        
      case MIDI_CHANNEL_SET_CH:  // Save pitch bend setting to EEPROM
        menu = SETTINGS;
        EEPROM.write(ADDR_MASTER_CHAN, masterChan);
        break;
        
    }
  }
  else { // Highlight wasn't visible, reinitialize highlight timer
    highlightTimer = millis();
    highlightEnabled = true;
  }
  encoderPos = 0;  // Reset encoder position
  encoderPosPrev = 0;
  updateSelection(); // Refresh screen
}

void updateSelection() { // Called whenever encoder is turned
  display.clearDisplay();
  switch (menu) {
    case KEYBOARD_MODE_SET_CH:
            if ((encoderPos > encoderPosPrev) && (keyboardMode < 2))
            keyboardMode += 1;
        else if ((encoderPos < encoderPosPrev) && (keyboardMode > 0))
            keyboardMode -= 1;
// No break statement, continue through next case      
    case SCALE_FACTOR_SET_CH:
      if ((encoderPos > encoderPosPrev) && (sfAdj < 1.1))
            sfAdj += 0.001f;
        else if ((encoderPos < encoderPosPrev) && (sfAdj > 0.9))
            sfAdj -= 0.001f;
// No break statement, continue through next case
    case MIDI_CHANNEL_SET_CH:
      if ((encoderPos > encoderPosPrev) && (masterChan < 16))
            masterChan += 1;
        else if ((encoderPos < encoderPosPrev) && (masterChan > 0))
            masterChan -= 1;
// No break statement, continue through next case

      case SETTINGS:
      display.setCursor(0,0); 
      display.setTextColor(WHITE,BLACK);
      display.print(F("SETTINGS"));
      display.setCursor(0,16);
           
      if (menu == SETTINGS) setHighlight(0,5);
      display.print(F("Keyboard Mode "));
      if (keyboardMode == 0 ) display.println("Poly  ");
      if (keyboardMode == 1 ) display.println("Unison");   
      if (keyboardMode == 2 ) display.println("Mono  ");
      display.println(F("  "));
         
      if (menu == SETTINGS) setHighlight(1,5);
      else display.setTextColor(WHITE,BLACK);
      display.print(F("Scale Factor  "));
      display.println(sfAdj,3);
      display.println(F("  "));
      
      if (menu == SETTINGS) setHighlight(2,5);
      display.print(F("Midi Channel  "));
      if (masterChan == 0) display.println("Omni ");
        else { display.println(masterChan);
        }
      break;     
  }
  display.display(); 
}

void setHighlight(int menuItem, int numMenuItems) {
  if ((mod(encoderPos, numMenuItems) == menuItem) &&  highlightEnabled ) {
    display.setTextColor(BLACK,WHITE);
  }
  else {
    display.setTextColor(WHITE,BLACK);
  }
}

int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}
