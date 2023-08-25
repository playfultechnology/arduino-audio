# arduino-audio
Notes, code, and wiring diagrams for common audio modules for small electronic projects

## Summary

| Image | Description | Interface | Storage | Power | Notes  | Purchase |
| :------------- | :------------ | :------------ | :------------ | :------------ | :------------ | :------------ |
| ![](https://sc04.alicdn.com/kf/HTB1ef5LKpXXXXXsXVXXq6xXFXXXs.jpg) | GPD2846 | Multifunction pads (Play/Pause/Advance/Previous/Vol Up/Down) | SD Card | 2W | 5V, auto-plays and loops all MP3 cards present on SD Card as soon as powered-up. Can play/pause and advance to next track by shorting input pins to GND, but no way to select particular tracks. Onboard 2W amp. Can be used for ambient BGM.  | https://www.aliexpress.com/item/32859062476.html |
| ![](https://images-na.ssl-images-amazon.com/images/I/612z-HkJB4L._AC_SL1000_.jpg) | YX5200 / YX5300 / YX6200 / YX6300 | Serial | SD Card | N/A | Controlled by simple set of serial commands on Tx/Rx pins. Can select particular tracks/folders (by number only not by name), start/stop etc. No on-board amp. https://arduinoplusplus.wordpress.com/2018/07/23/yx5300-serial-mp3-player-catalex-module/ | https://www.banggood.com/custlink/GmKYafRk4t |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/dfplayer.png) | YX5200 / YX5300  | Serial / IO / ADKey | SD Card | 3W | Goes by many names - most commonly "DFPlayer Mini", but also "MP3-TF-16P" and others. Typically combines YX5200 chip as above with YX8002-8S 3W amp. Try to avoid boards that come with MH2024K-24SS / JC AA20HF J616-94 clones, which although offer similar funcitonality many people have reported are fiddly to get to work - see https://www.thebackshed.com/forum/ViewTopic.php?TID=11977&P=1#164307 for a description of differences | https://www.banggood.com/custlink/GKDyjTR24w |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/dy-sv17f.png) | DY-SV17F | Serial / IO | 4Mb onboard flash | 5W | 4Mb flash memory, which can be triggered by serial commands or where 8 IO pins can be used to trigger 8 corresponding sound files. Requires some additional components (pull up/down resistors on mode select lines) |  |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/dy-sv8f.png) | DY-SV8F | Serial / IO | 8Mb onboard flash | 5W | As above, except has 8Mb onboard flash memory, 3.5mm headphone jack socket, and DIP switches allowing for mode to be set requiring no additional components | |
| ![](https://ae01.alicdn.com/kf/H7a4641c43db8424e82b8b128d81c65e8t/For-Arduino-MP3-Voice-Playback-Module-Music-Player-UART-I-O-Trigger-Amplifier-Class-D-5W.jpg_Q90.jpg) | DY-SV5W | Serial / IO | SD Card | 5W | Similar to above, except has SD card slot and DIP switches for mode-select so requires no additional components. https://grobotronics.com/images/companies/1/datasheets/DY-SV5W%20Voice%20Playback%20ModuleDatasheet.pdf?1559812879320 | https://www.banggood.com/custlink/vKGdlfhkz8 |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/dy-hv20t.png) | DY-HV20T | Serial / IO | SD Card | 20W | Same as DY-SV5W except has louder, 20W amp, and operates with supply voltage from 6-35V |  |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/dy-hv8f.png) | DY-HV8F | Serial / IO | 8Mb onboard flash | 20W | Same as DY-SV8F except has louder, 20W amp, and operates with supply voltage from 6-35W |  |
| ![](https://imgaz1.staticbg.com/thumb/large/oaupload/banggood/images/76/B2/95d112c6-fbc6-40f6-a313-3bfc68423d2f.jpg) | VLSI VS1053 | SPI | Varies | N/A | Supports very wide range of audio encodings (MP3, AAC, Ogg Vorbis, WMA, MIDI, FLAC, WAV (PCM and ADPCM)), together with MIDI support and microphone audio recording. Can playback and mix multiple simultaneous audio files. SPI interface, requires library and Arduino controller. Another advantage of these boards is they expose full functionality of the SD card - you can list folder structure, enumerate files, and reference files by "name", rather than just arbitrary index position. | https://www.banggood.com/custlink/mGDEo7yMso (Breakout with SD)    https://www.banggood.com/custlink/KDKRowRznd  (Breakout *no* SD card) https://www.banggood.com/custlink/vmvyl1RMA0 (Shield for UNO with SD card |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/robsertsonics_wavtrigger.png) | ARMSTM32 | | SD Card | - | RobertSonics WavTrigger | https://robertsonics.com/wav-trigger/ |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/adafruit_audiofx_soundboard.png) | VLSI VS1000D | | | - | Adafruit Audio FX | https://www.adafruit.com/product/2133 |
| ![](https://raw.githubusercontent.com/playfultechnology/arduino-audio/main/Docs/adafruit_audiofx_mini_soundboard.png) | VLSI VS1000D | | | - | Adafruit Audio Mini FX | https://www.adafruit.com/product/2342 |


## DY-SV17F
Note that this device operates at 3.3V logic! 5V is connected at pin 13, but all the I/O pins (whether using UART mode, trigger mode, or one_line mode), and the CON configuration pins should only ever have 3.3V signals written to them. The BUSY (pin 12) signal will also only output 3.3V logic HIGH.
If using a 5V MCU like an Arduino, a 5V->3.3V level shifter, or 1kΩ resistor _must_ be placed on any lines that are driven high by the MCU.
Additionally, the CON pins must be set through 10kΩ resistors through to GND (for LOW), or 3.3V (for HIGH).
- Datasheet at <a href="Docs/SEN-17-096 DataSheet.pdf">SEN-17-096 DataSheet.pdf</a>.
- Library at https://github.com/SnijderC/dyplayer

| Pin      | Connect to                                 |
| :------- | :----------------------------------------- |
| `Tx/IO0` | `MCU Rx` (via `1kΩ` res. if using 5V board |
| `Rx/IO1` | `MCU Tx` (via `1kΩ` res. if using 5V board |
| `IO2`    | -                                          |
| `IO3`    | -                                          |
| `IO4`    | -                                          |
| `IO5`    | -                                          |
| `IO6`    | -                                          |
| `IO7`    | -                                          |
| `GND`    | `GND`                                      |

| Pin      | Connect to                                 |
| :------- | :----------------------------------------- |
| `SPK+`   | Speaker +ve, if using direct speaker conn  |
| `SPK-`   | Speaker -ve, if using direct speaker conn  |
| `DACL`   | Left channel if using amp                  |
| `DACR`   | Right channel if using amp                 |
| `V33`    | 3.3V output from onboard LDO               |
| `V5`     | `5V`                                       |
| `CON3`   | `3.3V` (via `1kΩ` res)                     |
| `CON2`   | `GND` (via `1kΩ` res)                      |
| `CON1`   | `GND` (via `1kΩ` res)                      |

## DY-SV5W / DY-SV8F

## DY-HV20T / DY-HV8F

## DF Player Mini

There are _many_ DFPlayer Mini libraries, here are just a few:
- https://github.com/DFRobot/DFRobotDFPlayerMini
- https://github.com/PowerBroker2/DFPlayerMini_Fast
- https://github.com/Makuna/DFMiniMp3
- https://github.com/enjoyneering/DFPlayer

They all expose basically the same functionality, which is to create a 9600bps software serial connection (either using Arduino's built-in SoftwareSerial library, or an alternative like AltSoftSerial), and then writing command bytes to it. 
The format used by the DFPlayer is pretty straightforward:
```
0x7E FF 06 CMD FB LB HB CHK1 CHK0 0xEF
```
Where:
- 0x7E is the start byte
- FF is the version
- 06 is the length of the packet (excluding start byte and end byte)
- CMD is the command byte
- FB is the feedback byte (usually set to 0x00)
- LB and HB are the low and high bytes of parameters
- CHK1 and CHK0 are the high and low bytes of the checksum
- 0xEF is the end byte.

In fact, it's relatively straightforward to write your own code with no need for external library at all. For example, the following code will play track 4:

```
// Start Byte
Serial1.write(0x7E);

// Version
Serial1.write(0xFF);

// Command Length
Serial1.write(0x06);
  
// Command to playback a specific track
Serial1.write(0x03);
  
// Feedback byte
Serial1.write(0x00);
  
// Parameter 1 (High byte of track number)
Serial1.write(0x00);
  
// Parameter 2 (Low byte of track number)
Serial1.write(0x04);
  
// Checksum calculated as 0 - (0x06 + command + 0x00 + param1 + param2);
Serial1.write(0x00);
Serial1.write(0xF3);
  
// End Byte
Serial1.write(0xEF);

```

Find out what functionality the chip on your DFPlayer supports:
- https://github.com/ghmartin77/DFPlayerAnalyzer

## Serial MP3 Player

Playback libraries
---
https://github.com/salvadorrueda/SerialMP3Player
https://github.com/MajicDesigns/MD_YX5300


