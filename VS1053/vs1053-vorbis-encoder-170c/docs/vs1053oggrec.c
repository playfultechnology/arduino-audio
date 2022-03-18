/*

  Example code how to implement VS1053 Ogg Vorbis recording using
  the VS1053 Ogg Vorbis Recording plugin. The code is generic enough
  so that it should pretty much compile and run on any microcontroller.

  The code makes the following assumptions:
  - <stdio.h> functions fopen(), fclose(), fgetc() and fputc() are available
  - The following VS1053 interfacing functions must exist:
    - void Write1053Sci(int regNo, u_int16 value): writes value to SCI register
    - u_int16 Read1053Sci(int regNo): reads value from SCI register
    - WaitFor1053Dreq(int timeOut): Waits for 1 microsecond, then until
      DREQ goes up or timeOut is reached (timeOut=1 is 10 ms).
  - EndRecording() function must exist, which returns non-zero when the user
    has requested that recording should be finished.
      
  NOTE!
  This code serves as an example of how to create generic C microcontroller
  code for VS1053 recording.

  Author: Henrik Herranen / VLSI Solution 2011.
  Copyright: Use freely for any project with any VLSI Solution's IC.
  Warranty: Absolutely none whatsoever.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Change these if they don't match you architecture. */
typedef unsigned short u_int16;
typedef short          s_int16;
typedef unsigned long  u_int32;
typedef long           s_int32;

#define SCIR_MODE        0
#define SCIR_STATUS      1
#define SCIR_BASS        2
#define SCIR_CLOCKF      3
#define SCIR_DECODE_TIME 4
#define SCIR_AUDATA      5
#define SCIR_WRAM        6
#define SCIR_WRAMADDR    7
#define SCIR_HDAT0       8
#define SCIR_HDAT1       9
#define SCIR_AIADDR     10
#define SCIR_VOL        11
#define SCIR_AICTRL0    12
#define SCIR_AICTRL1    13
#define SCIR_AICTRL2    14
#define SCIR_AICTRL3    15


/* returns minimum of two given numbers */
#define min(a,b) (((a)<(b))?(a):(b))


/* Loads an image file into VS1053 from a file. */
auto u_int16 SpiLoadImageInto1053(FILE *fp);


/* Main recording program */
void main(void) {
  FILE *outFP, *inFP;
  u_int16 pluginStartAddr;
  u_int16 state = 0;
  u_int16 i;
  u_int16 wordsToRead;
  u_int16 wordsWaiting;

  /* Perform Microcontroller specific initializations here. */



  /* Following VS1053 operations are according to the
     VS1053b Ogg Vorbis Encoder application manual. For
     details read Chapter Loading and Starting the Code. */

  /* Set VS1053 clock to 4.5x = 55.3 MHz */
  Write1053Sci(SCIR_CLOCKF, 0xC000);
  WaitFor1053Dreq(TIMER_TICKS/10);

  /* Clear SCI_BASS */
  Write1053Sci(SCIR_BASS, 0);

  /* Reset VS1053 */
  Write1053Sci(SCIR_MODE, SMF_SDINEW | SMF_RESET);
  WaitFor1053Dreq(TIMER_TICKS/10);      /* Wait until DREQ is high or 100 ms */

  // Disable all interrupts except SCI
  Write1053Sci(SCIR_WRAMADDR, VS1053_INT_ENABLE);
  Write1053Sci(SCIR_WRAM, 0x2);

  /* Load the recorder application to VS1053
     This source code uses .img image files for loading.
     If you use .plg files, use the source as described
     in the VS1053b Ogg Vorbis Encoder application manual. */
  inFP = fopen("venc44k2q05.img", "rb");
  if (!inFP)
    goto end;
  pluginStartAddr = SpiLoadImageInto1053(inFP);
  fclose(inFP);

  /* If loading failed, give up. */
  if (pluginStartAddr == 0xFFFF)
    goto end;

  /*  Now open output file. It's better to do this before activating
      recording so that even if opening the output file is slow,
      you will not lose audio data. */
  outFP = fopen("record.ogg", "wb");
  if (!outFP)
    goto end;

  /* Set VS1053 mode bits as instructed in the VS1053b Ogg Vorbis Encoder
     manual. Note: for microphone input, leave SMF_LINE1 unset! */
  Write1053Sci(SCIR_MODE, SMF_LINE1 | SMF_ADPCM | SMF_SDINEW);

  /* Rec level: 1024 = 1. If 0, use AGC */
  Write1053Sci(SCIR_AICTRL1, 1024);
  /* Maximum AGC level: 1024 = 1. Only used if SCI_AICTRL1 is set to 0. */
  Write1053Sci(SCIR_AICTRL2, 0);
  /* Miscellaneous bits that also must be set before recording. */
  Write1053Sci(SCIR_AICTRL3, 0);

  /* Activate recording from the address we got. (In the case of the Ogg
     Vorbis Encoder application, pluginStartAddr = 0x34.) */
  Write1053Sci(SCIR_AIADDR, pluginStartAddr);
  WaitFor1053Dreq(TIMER_TICKS/10);


  /*
    By now we have:
    - Initialized the VS1053
    - Loaded the VS1053 recording plugin
    - Opened the output file
    - Activated the VS1053 recording plugin

    So, now it's time to record. Below is the recording loop.

    The variable "state" controls the progression of recording as follows:
    state = 0: Normal recording.
    state = 1: User has requested end of recording, microcontroller has
        also requested this from the VS1053 recording plugin.
    state = 2: The VS1053 plugin has stopped recording, but the
        microcontroller is still collecting data from the VS1053 buffers.
    state = 3: Recording finished.
  */


  /* Main loop */
  while (state < 3) {

    /* Check when to end recording. The function EndRecording() is
       microcontroller specific and should be implementing separately
       for each application. */
    if (EndRecording() && !state) {
      state = 1;
      Write1053Sci(SCIR_AICTRL3, 1); // Send VS1053 request to stop recording
    }

    /* See how many 16-bit words there are waiting in the VS1053 buffer */
    wordsWaiting = Read1053Sci(SCIR_HDAT1);

    /* If user has requested stopping recording, and VS1053 has
       stopped recording, proceed to the next state. */
    if (state == 1 && Read1053Sci(SCIR_AICTRL3) & (1<<1)) {
      state = 2;
      /* It is important to reread the HDAT1 register once after
         VS1053 has stopped. Otherwise there is the chance that
         a few more words have just arrived although we just
         read this register. So, do NOT optimize the following
         line away! */
      wordsWaiting = Read1053Sci(SCIR_HDAT1);
    }

    /* Read and transfer whole 512-byte (256-word) disc blocks at
       the time. The only exception is when recording ends: then
       allow for a non-full block. */
    while (wordsWaiting >= ((state < 2) ? 256 : 1)) {
      wordsToRead = min(wordsWaiting, 256);
      wordsWaiting -= wordsToRead;

      /* If this is the very last block, read one 16-bit word less,
         because it will be handled later separately. */
      if (state == 2 && !wordsWaiting)
        wordsToRead--;

      /* Transfer one full data block, or if this is the very last
         block, all data that's left except for the last word. */
      {
        u_int16 t;
        u_int16 i;
        for (i=0; i<wordsToRead; i++) {
          t = Read1053Sci(SCIR_HDAT0);
          fputc(t >> 8  , outFP);
          fputc(t & 0xFF, outFP);
        }
      }

      /* If this is the last data block... */
      if (wordsToRead < 256) {
        u_int16 lastWord;
        state = 3;

        /* ... read the very last word of the file */
        lastWord = Read1053Sci(SCIR_HDAT0);

        /* Always write first half of the last word. */
        fputc(lastWord >> 8  , outFP);

        /* Read twice SCIR_AICTRL3, then check bit 2 of latter read. */
        Read1053Sci(SCIR_AICTRL3);
        if (!(Read1053Sci(SCIR_AICTRL3) & (1<<2))) {
          /* Write last half of the last word only if bit 2 is clear. */
          fputc(lastWord & 0xFF, outFP);
        }
      } /* if (wordsToRead < 256) */
    } /* while (wordsWaiting >= ((state < 2) ? 256 : 1)) */

  } /* while (state < 3), end of main loop */


  /* That's it! We've perfectly recorded an Ogg Vorbis file, so now
     we only need to close the file and be happy about it. */
  fclose(outFP);


 end:
  /* Finally, reset VS1053 so that we will hear no more monitor audio */
  Write1053Sci(SCIR_MODE, SMF_SDINEW | SMF_RESET);

  /* End up in infinite loop */
  while (1)
    ;
}





/*
  SpiLoadImageInto1053() loads an image from a file into the memory
  of VS1053(), then returns the start address to the caller. If it
  fails, it will return 0xFFFFU.

  The file format is as follows:

  The file starts with three characters, "P&H".

  The rest of the file are followed by records that are in the
  following format:
  Tp  L1 L0  A1 A0  D0 D1 D2 ....

  Tp: The type of the record. Values 0..3 are valid:
  - 0 = Code (Instructions)
  - 1 = X memory
  - 2 = Y memory
  - 3 = Execute (start address)

  (L1<<8) + L0: Length of the record in bytes. Always an even number.

  (A1<<8) + A0: Start address of the record. If the record is of type
    Execute, then this is the execution start address. It is also the
    end of the file.

  Dx: Data. Read this two bytes at a time and send to VS1053 in a
    big-endian fashion, as shown in the code below.
*/
auto u_int16 SpiLoadImageInto1053(FILE *fp) {
  s_int16 type;

  if (fgetc(fp) != 'P' || fgetc(fp) != '&' || fgetc(fp) != 'H') {
    return 0xFFFF;
  }

  while ((type = fgetc(fp)) >= 0) {
    /*
      offsets are: Code (I), X Mem, Y Mem when written through SCI_WRAM.
      See VS1053 datasheet's documentation for SCI register SCI_WRAMADDR for
      details.
    */
    static u_int16 offsets[3] = {0x8000U, 0x0U, 0x4000U};
    u_int16 len, addr;

    if (type >= 4) {
      /* Error condition */
      return 0xFFFF;
    }

    len = (u_int16)fgetc(fp) << 8;
    len |= fgetc(fp) & ~1;
    addr = (u_int16)fgetc(fp) << 8;
    addr |= fgetc(fp);
    if (type == 3) {
      /* Execute record: we can now return with the execute address */
      return addr;
    }

    /* Set address */
    Write1053Sci(SCIR_WRAMADDR, addr + offsets[type]);

    /* Write data */
    do {
      u_int16 data;
      data = (u_int16)fgetc(fp) << 8;
      data |= fgetc(fp);
      Write1053Sci(SCIR_WRAM, data);
    } while ((len -= 2));
  }
  return 0xFFFF;
}
