/*
 LCD Display memory format - 4x byte (32-bit)
  00xaxaxa, 00fbfbfb, 00egegeg, 00dcdcdc
 Glyph format - 1x byte (8-bit)
  dcegfbxa
*/

// Glyph (packed) format
#define SEG_A 0x1
#define SEG_X 0x2
#define SEG_B 0x4
#define SEG_F 0x8
#define SEG_G 0x10
#define SEG_E 0x20
#define SEG_C 0x40
#define SEG_D 0x80

#define DIG_MASK 3

// a,b,c,g digit 3 = bit 0
// f,e,d digit 1 = bit 5
#define DIG_1 4
#define DIG_2 2
#define DIG_3 0

#define BLANK 0
#define DASH SEG_G

typedef byte lcd_glyph;

const lcd_glyph numeral[] = {
  /*0*/ SEG_A + SEG_B + SEG_C + SEG_D + SEG_E + SEG_F        ,
  /*1*/         SEG_B + SEG_C                                ,
  /*2*/ SEG_A + SEG_B +         SEG_D + SEG_E +         SEG_G,
  /*3*/ SEG_A + SEG_B + SEG_C + SEG_D +                 SEG_G,
  /*4*/ SEG_B +         SEG_C +                 SEG_F + SEG_G,
  /*5*/ SEG_A +         SEG_C + SEG_D +         SEG_F + SEG_G,
  /*6*/ SEG_A +         SEG_C + SEG_D + SEG_E + SEG_F + SEG_G,
  /*7*/ SEG_A + SEG_B + SEG_C                                ,
  /*8*/ SEG_A + SEG_B + SEG_C + SEG_D + SEG_E + SEG_F + SEG_G,
  /*9*/ SEG_A + SEG_B + SEG_C + SEG_D +         SEG_F + SEG_G
};

// TODO
const lcd_glyph alpha[] = {
  /*a*/ BLANK,
  /*b*/                 SEG_C + SEG_D + SEG_E + SEG_F + SEG_G,
  /*c*/                         SEG_D + SEG_E +         SEG_G,
  /*d*/         SEG_B + SEG_C + SEG_D + SEG_E +         SEG_G,
  /*e*/ BLANK,
  /*f*/ SEG_A +                         SEG_E + SEG_F + SEG_G,
  /*g*/ SEG_A + SEG_B + SEG_C + SEG_D +         SEG_F + SEG_G,
  /*h*/ BLANK,
  /*i*/ BLANK,
  /*j*/ BLANK,
  /*k*/ BLANK,
  /*l*/ BLANK,
  /*m*/ BLANK,
  /*n*/ BLANK,
  /*o*/ BLANK,
  /*p*/ BLANK,
  /*q*/ BLANK,
  /*r*/ BLANK,
  /*s*/ BLANK,
  /*t*/ BLANK,
  /*u*/ BLANK,
  /*v*/ BLANK,
  /*w*/ BLANK,
  /*x*/ BLANK,
  /*y*/ BLANK,
  /*z*/ BLANK,
};

inline static lcd_glyph lcd_char(const char c) {
  if(c < '0') return BLANK;
  if(c <= '9') return numeral[c - '0'];
  if(c < 'A') return BLANK;
  if(c <= 'Z') return alpha[c - 'A'];
  if(c < 'a') return BLANK;
  if(c <= 'z') return alpha[c - 'a'];
  return BLANK;
}
