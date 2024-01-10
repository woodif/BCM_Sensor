
// Custom "Adafruit" compatible font files can be added to the "TFT_eSPI/Fonts/Custom" folder
// Fonts in a suitable format can be created using a Squix blog web based tool here:
/*
   https://blog.squix.org/2016/10/font-creator-now-creates-adafruit-gfx-fonts.html
*/

// Note: At the time of writing there is a last character code definition bug in the
// Squix font file format so do NOT try and print the tilda (~) symbol (ASCII 0x7E)
// Alternatively look at the end of the font header file and edit:  0x7E to read 0x7D
/* e.g.                                                                          vvvv
  (uint8_t  *)Orbitron_Light_32Bitmaps,(GFXglyph *)Orbitron_Light_32Glyphs,0x20, 0x7D, 32};
                                                                                 ^^^^
*/

// When font files are placed in the Custom folder (TFT_eSPI\Fonts\Custom) then they must
// also be #included here:

// The CF_OL24 etc are a shorthand reference, but this is not essential to use the fonts

#ifdef LOAD_GFXFF

  // New custom font file #includes
  #include <Fonts/Custom/Orbitron_Light_24.h> // CF_OL24
  #include <Fonts/Custom/Orbitron_Light_32.h> // CF_OL32
  #include <Fonts/Custom/Roboto_Thin_24.h>    // CF_RT24
  #include <Fonts/Custom/Satisfy_24.h>        // CF_S24
  #include <Fonts/Custom/Yellowtail_32.h>     // CF_Y32

    // Kanit
  #include <Fonts/Custom/Kanit_Bold_9_en.h>  // FF33 or FS9
  #include <Fonts/Custom/Kanit_Bold_12_en.h> // FF34 or FS12
  #include <Fonts/Custom/Kanit_Bold_18_en.h> // FF35 or FS18
  #include <Fonts/Custom/Kanit_Bold_24_en.h> // FF36 or FS24
  
  #include <Fonts/Custom/Kanit_Bold_9_th.h>  // FF37 or FSI9
  #include <Fonts/Custom/Kanit_Bold_12_th.h> // FF38 or FSI12
  #include <Fonts/Custom/Kanit_Bold_18_th.h> // FF39 or FSI18
  #include <Fonts/Custom/Kanit_Bold_24_th.h> // FF40 or FSI24

#endif

// Shorthand references - any coding scheme can be used, here CF_ = Custom Font
// The #defines below MUST be added to sketches to use shorthand references, so
// they are only put here for reference and copy+paste purposes!
/*
#define CF_OL24 &Orbitron_Light_24
#define CF_OL32 &Orbitron_Light_32
#define CF_RT24 &Roboto_Thin_24
#define CF_S24  &Satisfy_24
#define CF_Y32  &Yellowtail_32
*/
