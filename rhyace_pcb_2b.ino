#include <EEPROM.h>

#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

#include <SPI.h>
SPISettings adc1(20000000, MSBFIRST, SPI_MODE0);

#include <FastLED.h>
#define NUM_LEDS 7
#define DATA_PIN 17
CRGB leds[NUM_LEDS];


const int cs1 = 10;
byte order[9] = { 7, 0, 1, 2, 3, 4, 5, 6 };
byte mapshi[9] = { 127, 127, 127, 127, 127, 127, 127, 127 };
byte mapslo[9] = { 0, 0, 56, 56, 5, 10, 2, 0 };
byte mapmid[9] = { 56, 70, 78, 84, 38, 38, 44, 84 };
unsigned long current_time;
unsigned long prev_time[8];
uint32_t cu, du, cu2, du2;

byte br[40];
int signs[4];
int reads[2];
int preads[2];
byte d7, pd7;
int wiggle;
int numwig;
byte add, wb, fs;

byte osc1;
float snap[9];
int p1;
int osc2;
byte tick;
byte tock;
byte lfos[8];
int type, note, velocity, channel, d1, d2;

int latchPin = 3;
int clockPin = 2;
int dataPin = 4;
byte sw_out0;
byte mmode = 0;
#define rly0 7
#define clken 8
float lblink;
float lba = 5;
byte lolvl = 40;
byte hilvl = 70;
byte hhh;
byte dinch = 1;
byte firstdin = 1;

void setup() {

  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
  for (byte j = 0; j < NUM_LEDS; j++) {
    leds[j].setHSV(0, 0, 50);
  }
  FastLED.show();
  delay(250);
  for (byte j = 0; j < NUM_LEDS; j++) {
    leds[j].setHSV(0, 0, 0);
  }
  FastLED.show();


  Serial.begin(9600);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();

  pinMode(cs1, OUTPUT);
  digitalWrite(cs1, LOW);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clken, OUTPUT);
  pinMode(rly0, OUTPUT);

  digitalWrite(clken, 1);
  digitalWrite(rly0, 0);

  delay(10);




  Serial.println(" hi ");
  dacBegin();
  for (byte j = 0; j < 8; j++) {
    dacw(j, 0);
  }
  lblink = 0;
  FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
  for (byte j = 0; j < NUM_LEDS; j++) {
    leds[j].setHSV(0, 0, 50);
  }
  FastLED.show();
  delay(250);
  for (byte j = 0; j < NUM_LEDS; j++) {
    leds[j].setHSV(0, 0, 0);
  }
  FastLED.show();
}


void loop() {
  current_time = micros();

  if (MIDI.read()) {
    byte type = MIDI.getType();
    d1 = MIDI.getData1();
    d2 = MIDI.getData2();
    channel = MIDI.getChannel();
    //Serial.println(String("DIN ") + type + " " + d1 + " " + d2 + " " + channel);
    //Serial.println(String("dinch ") + dinch);
    if (type == 144) {  //on
      if (firstdin == 1) {
        firstdin = 0;
        dinch = channel;
      }
      if (channel == dinch) {
        notehappen(d1, d2);
      }
    }
  }

  if (usbMIDI.read()) {
    byte type = usbMIDI.getType();
    d1 = usbMIDI.getData1();
    d2 = usbMIDI.getData2();

    //Serial.println(String("USB ") + type + " " + d1 + " " + d2);
    if (type == 144) {
      notehappen(d1, d2);
    }
  }


  if (current_time - prev_time[0] > 1000 && 1) {
    prev_time[0] = current_time;
    for (byte j = 0; j < 8; j++) {
      dacw(j, snap[j] * 255.0);
      snap[j] *= .7;
      if (snap[j] < .01) {
        snap[j] = 0;
      }
    }
  }

  if (current_time - prev_time[1] > 10 * 1000 && mmode == 1) {
    prev_time[1] = current_time;
    if (lblink > 1) {
      lblink--;
      leds[2].setHSV(0, 0, hilvl * (lblink / lba));
      leds[5].setHSV(0, 0, hilvl * (lblink / lba));
    } else {
      hhh++;
      leds[2].setHSV(0, 0, lolvl);
      leds[5].setHSV(0, 0, lolvl);
    }
    FastLED.show();
  }


  if (current_time - prev_time[2] > 5000 * 1000 && 0) {
    prev_time[2] = current_time;
    Serial.print(tempmonGetTemp());
    Serial.println("°C");
  }
}

void dacw(byte sel, int val) {
  if (val > 255) { val = 255; }
  if (val < 0) { val = 0; }

  byte cmd1 = (sel << 3);
  //cu = micros();
  SPI.beginTransaction(adc1);
  digitalWrite(cs1, LOW);
  SPI.transfer(cmd1);
  SPI.transfer(0);
  SPI.transfer(val);
  digitalWrite(cs1, HIGH);
  SPI.endTransaction();
}

void dacBegin() {

  add = 0x09;
  wb = 0;
  fs = (add << 3) + wb;
  SPI.begin();
  SPI.beginTransaction(adc1);
  digitalWrite(cs1, LOW);
  SPI.transfer(fs);  //power
  SPI.transfer(0);
  SPI.transfer(0);
  digitalWrite(cs1, HIGH);
  SPI.endTransaction();
  delay(10);


  add = 0x08;
  wb = 0;
  fs = (add << 3) + wb;
  SPI.begin();
  SPI.beginTransaction(adc1);
  digitalWrite(cs1, LOW);
  SPI.transfer(fs);  //ref
  SPI.transfer(255);
  SPI.transfer(255);
  digitalWrite(cs1, HIGH);
  SPI.endTransaction();
  delay(10);

  add = 0x0B;
  wb = 0;
  fs = (add << 3) + wb;
  SPI.begin();
  SPI.beginTransaction(adc1);
  digitalWrite(cs1, LOW);
  SPI.transfer(fs);  //lock
  SPI.transfer(0);
  SPI.transfer(0);
  digitalWrite(cs1, HIGH);
  SPI.endTransaction();
  delay(10);
}

void notehappen(byte ind1, byte ind2) {

  digitalWrite(clken, 0);
  digitalWrite(rly0, 1);
  mmode = 1;
  lblink = lba;
  for (byte j = 0; j < 8; j++) {
    if (ind1 - 24 == j || (ind1 - 24) % 12 == j) {
      byte v1 = map(ind2, 0, 99, mapslo[j], mapmid[j]);
      byte v2 = map(ind2, 100, 127, mapmid[j], 127);

      byte t1 = order[j];
      if (v1 < mapmid[j]) {
        snap[t1] = v1 / 127.0;
      } else {
        snap[t1] = v2 / 127.0;
      }
      //Serial.print(j);          Serial.print(" ");          Serial.println(t1);
    }
  }
}
