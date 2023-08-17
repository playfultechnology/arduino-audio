/**
 * VS1053 Shield
 */

// DEFINES
#define RECBUFFSIZE 128  // 64 or 128 bytes.
//#define CLK 13       // SPI Clock, shared with SD card
//#define MISO 12      // Input data, from VS1053/SD card
//#define MOSI 11      // Output data, to VS1053/SD card
#define BREAKOUT_RESET  8      // VS1053 reset pin (output) marked XRESET
#define BREAKOUT_CS     6     // VS1053 chip select pin (output) marked XCS
#define BREAKOUT_DCS    7      // VS1053 Data/command select pin (output) marked XDCS
#define CARDCS 9     // Card chip select pin
#define DREQ 2       // VS1053 Data request, ideally an Interrupt pin

// INCLUDES
#include <SPI.h>
// For reading files stored on SD card
#include <SD.h>
// For interfacing with MP3 sound board
#include <Adafruit_VS1053.h>

// CONSTANTS
const byte recPin = 3;
const byte playbackPin = 4;

// GLOBALS
Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);
bool isPlaying = false;
uint8_t isRecording = false;
File saveFile;  // the file we will save our recording to
#define RECBUFFSIZE 128  // 64 or 128 bytes.
uint8_t recording_buffer[RECBUFFSIZE];

void setup() {
  Serial.begin(115200);
  Serial.println(__FILE__ __DATE__);

  Serial.print(F("Initialising VS1053..."));
  if(!musicPlayer.begin()) { // Initialise the VS1053 MP3 player
     Serial.println(F("FAILED. Check you have the correct pins defined"));
     while(1);
  }
  Serial.println("OK");

  delay(500);

  Serial.print(F("Initialising SD Card..."));
  if(!SD.begin(CARDCS)) {
    Serial.println(F("FAILED"));
    while (1);
  }
  Serial.println(F("OK"));

  delay(500);

  Serial.print(F("Loading VS1053 Firmware..."));
  // Load VS1053 firmware from SD card.
  // Different profiles can be downloaded from VS1053 Ogg Vorbis Encoder Application
  // http://www.vlsi.fi/en/support/software/vs10xxapplications.html
  // Note that filenames must follow the 8.3 filename convention
  // We'll use mono 44.1KHz, high quality
  // v16k1q05 is Wideband voice, mono, mid-quality
  if (! musicPlayer.prepareRecordOgg("v16k1q05.img")) {
     Serial.println("FAILED");
     while (1);    
  }
  Serial.println(F("OK"));

/*
  // List files
  printDirectory(SD.open("/"), 0);
  
  // Set volume for left, right channels. Lower numbers == louder volume!
  musicPlayer.setVolume(5,5);
*/

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can play audio in background
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

 // musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working  

  // Declare pinmodes
  pinMode(recPin, INPUT_PULLUP);

  musicPlayer.sineTest(0x44, 100);
  musicPlayer.sineTest(0x50, 100);
}

void loop() {
  
  if(!isRecording) {
    if(!digitalRead(recPin)) {
      Serial.println(F("Begin recording"));
      isRecording = true;
      
      // Check if the file exists already
      char filename[15];
      strcpy(filename, "RECORD00.OGG");
      for (uint8_t i = 0; i < 100; i++) {
        filename[6] = '0' + i/10;
        filename[7] = '0' + i%10;
        // create if does not exist, do not open existing, write, sync after write
        if (! SD.exists(filename)) {
          break;
        }
      }
      Serial.print(F("Recording to ")); Serial.println(filename);
      saveFile = SD.open(filename, FILE_WRITE);
      if (!saveFile) {
         Serial.println(F("Couldn't open file to record!"));
         while (1);
      }
      musicPlayer.startRecordOgg(true); // use microphone (for linein, pass in 'false')
    }
  }
  if(isRecording) {
    // Continuously save audio stream to SD card
    saveRecordedData(isRecording);
    
    if(digitalRead(recPin)) {
      Serial.println("End recording");
      musicPlayer.stopRecordOgg();
      isRecording = false;
      // flush all the data!
      saveRecordedData(isRecording);
      // close it up
      saveFile.close();
      delay(1000);
    }
  }
  
}

/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}


uint16_t saveRecordedData(boolean isrecord) {
  uint16_t written = 0;
  
    // read how many words are waiting for us
  uint16_t wordswaiting = musicPlayer.recordedWordsWaiting();
  
  // try to process 256 words (512 bytes) at a time, for best speed
  while (wordswaiting > 256) {
    Serial.print("Waiting: "); Serial.println(wordswaiting);
    // for example 128 bytes x 4 loops = 512 bytes
    for (int x=0; x < 512/RECBUFFSIZE; x++) {
      // fill the buffer!
      for (uint16_t addr=0; addr < RECBUFFSIZE; addr+=2) {
        uint16_t t = musicPlayer.recordedReadWord();
        //Serial.println(t, HEX);
        recording_buffer[addr] = t >> 8; 
        recording_buffer[addr+1] = t;
      }
      if (! saveFile.write(recording_buffer, RECBUFFSIZE)) {
            Serial.print("Couldn't write "); Serial.println(RECBUFFSIZE); 
            while (1);
      }
    }
    // flush 512 bytes at a time
    saveFile.flush();
    written += 256;
    wordswaiting -= 256;
  }
  
  wordswaiting = musicPlayer.recordedWordsWaiting();
  if (!isrecord) {
    Serial.print(wordswaiting); Serial.println(" remaining");
    // wrapping up the recording!
    uint16_t addr = 0;
    for (int x=0; x < wordswaiting-1; x++) {
      // fill the buffer!
      uint16_t t = musicPlayer.recordedReadWord();
      recording_buffer[addr] = t >> 8; 
      recording_buffer[addr+1] = t;
      if (addr > RECBUFFSIZE) {
          if (! saveFile.write(recording_buffer, RECBUFFSIZE)) {
                Serial.println("Couldn't write!");
                while (1);
          }
          saveFile.flush();
          addr = 0;
      }
    }
    if (addr != 0) {
      if (!saveFile.write(recording_buffer, addr)) {
        Serial.println("Couldn't write!"); while (1);
      }
      written += addr;
    }
    musicPlayer.sciRead(VS1053_SCI_AICTRL3);
    if (! (musicPlayer.sciRead(VS1053_SCI_AICTRL3) & (1 << 2))) {
       saveFile.write(musicPlayer.recordedReadWord() & 0xFF);
       written++;
    }
    saveFile.flush();
  }

  return written;
}
