// Converter tool from GoatTracker2 song format to minimal player
// Cadaver (loorni@gmail.com) 2/2018

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileio.h"
#include "gcommon.h"

#define MST_NOFINEVIB 0
#define MST_FINEVIB 1
#define MST_FUNKTEMPO 2
#define MST_PORTAMENTO 3
#define MST_RAW 4

#define WTBL 0
#define PTBL 1
#define FTBL 2
#define STBL 3

#define MAX_NTSONGS 16
#define MAX_NTPATT 127
#define MAX_NTCMD 127
#define MAX_NTPATTLEN 256
#define MAX_NTSONGLEN 256
#define MAX_NTTBLLEN 255

#define NT_ENDPATT 0x00
#define NT_FIRSTNOTE 0x02
#define NT_LASTNOTE 0x7a
#define NT_WAVEPTR 0x7c
#define NT_KEYOFF 0x7e
#define NT_REST 0x7f
#define NT_NOCMD 0x1;
#define NT_MAXDUR 128

INSTR instr[MAX_INSTR];
unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
unsigned char songorder[MAX_SONGS][MAX_CHN][MAX_SONGLEN+2];
unsigned char pattern[MAX_PATT][MAX_PATTROWS*4+4];
unsigned char patttempo[MAX_PATT][MAX_PATTROWS+1];
unsigned char pattinstr[MAX_PATT][MAX_PATTROWS+1];
unsigned char pattkeyon[MAX_PATT][MAX_PATTROWS+1];
unsigned char pattbasetrans[MAX_PATT];
unsigned char pattremaptempo[MAX_PATT];
unsigned char pattremapsrc[MAX_PATT];
unsigned char pattremapdest[MAX_PATT];

char songname[MAX_STR];
char authorname[MAX_STR];
char copyrightname[MAX_STR];
int pattlen[MAX_PATT];
int songlen[MAX_SONGS][MAX_CHN];
int tbllen[MAX_TABLES];
int highestusedpatt;
int highestusedinstr;
int highestusedsong;
int defaultpatternlength = 64;
int remappedpatterns = 0;
int maxdur = NT_MAXDUR;

unsigned char ntwavetbl[MAX_NTTBLLEN+1];
unsigned char ntnotetbl[MAX_NTTBLLEN+1];
unsigned char ntnexttbl[MAX_NTTBLLEN+1];
unsigned char ntpulselimittbl[MAX_NTTBLLEN+1];
unsigned char ntpulsespdtbl[MAX_NTTBLLEN+1];
unsigned char ntpulsenexttbl[MAX_NTTBLLEN+1];
unsigned char ntfiltlimittbl[MAX_NTTBLLEN+1];
unsigned char ntfiltspdtbl[MAX_NTTBLLEN+1];
unsigned char ntfiltnexttbl[MAX_NTTBLLEN+1];

unsigned char ntpatterns[MAX_NTPATT][MAX_NTPATTLEN];
unsigned char ntpattlen[MAX_NTPATT];

unsigned char nttracks[MAX_NTSONGS][MAX_NTSONGLEN];
unsigned char ntsongstart[MAX_NTSONGS][3];
unsigned char ntsongtotallen[MAX_NTSONGS];

unsigned char ntcmdad[MAX_NTCMD];
unsigned char ntcmdsr[MAX_NTCMD];
unsigned char ntcmdwavepos[MAX_NTCMD];
unsigned char ntcmdpulsepos[MAX_NTCMD];
unsigned char ntcmdfiltpos[MAX_NTCMD];

int ntcmdsize = 0;
int ntwavesize = 0;
int ntpulsesize = 0;
int ntfiltsize = 0;

FILE* out = 0;

unsigned short freqtbl[] = {
    0x022d,0x024e,0x0271,0x0296,0x02be,0x02e8,0x0314,0x0343,0x0374,0x03a9,0x03e1,0x041c,
    0x045a,0x049c,0x04e2,0x052d,0x057c,0x05cf,0x0628,0x0685,0x06e8,0x0752,0x07c1,0x0837,
    0x08b4,0x0939,0x09c5,0x0a5a,0x0af7,0x0b9e,0x0c4f,0x0d0a,0x0dd1,0x0ea3,0x0f82,0x106e,
    0x1168,0x1271,0x138a,0x14b3,0x15ee,0x173c,0x189e,0x1a15,0x1ba2,0x1d46,0x1f04,0x20dc,
    0x22d0,0x24e2,0x2714,0x2967,0x2bdd,0x2e79,0x313c,0x3429,0x3744,0x3a8d,0x3e08,0x41b8,
    0x45a1,0x49c5,0x4e28,0x52cd,0x57ba,0x5cf1,0x6278,0x6853,0x6e87,0x751a,0x7c10,0x8371,
    0x8b42,0x9389,0x9c4f,0xa59b,0xaf74,0xb9e2,0xc4f0,0xd0a6,0xdd0e,0xea33,0xf820,0xffff
};

void loadsong(const char* songfilename);
void clearsong(int cs, int cp, int ci, int cf, int cn);
void clearinstr(int num);
void clearpattern(int num);
void countpatternlengths(void);
int makespeedtable(unsigned data, int mode, int makenew);
void printsonginfo(void);
void clearntsong(void);
void convertsong(void);
void getpatttempos(void);
void getpattbasetrans(void);
void saventsong(const char* songfilename);
void writeblock(FILE* out, const char* blockname, unsigned char* data, int len);

int main(int argc, const char** argv)
{
    if (argc < 3)
    {
        printf("Converter from GT2 format to minimal player. Outputs source code.\n"
               "Usage: gt2mini <input> <output>");
        return 1;
    }

    loadsong(argv[1]);
    printsonginfo();
    clearntsong();
    convertsong();
    saventsong(argv[2]);
    return 0;
}

void loadsong(const char* songfilename)
{
    int c;
    int ok = 0;
    char ident[4];
    FILE *handle;

    handle = fopen(songfilename, "rb");

    if (!handle)
    {
        printf("Could not open input song %s\n", songfilename);
        exit(1);
    }

    fread(ident, 4, 1, handle);
    if ((!memcmp(ident, "GTS3", 4)) || (!memcmp(ident, "GTS4", 4)) || (!memcmp(ident, "GTS5", 4)))
    {
      int d;
      int length;
      int amount;
      int loadsize;
      clearsong(1,1,1,1,1);
      ok = 1;

      // Read infotexts
      fread(songname, sizeof songname, 1, handle);
      fread(authorname, sizeof authorname, 1, handle);
      fread(copyrightname, sizeof copyrightname, 1, handle);

      // Read songorderlists
      amount = fread8(handle);
      highestusedsong = amount - 1;
      for (d = 0; d < amount; d++)
      {
        for (c = 0; c < MAX_CHN; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          fread(songorder[d][c], loadsize, 1, handle);
        }
      }
      // Read instruments
      amount = fread8(handle);
      for (c = 1; c <= amount; c++)
      {
        instr[c].ad = fread8(handle);
        instr[c].sr = fread8(handle);
        instr[c].ptr[WTBL] = fread8(handle);
        instr[c].ptr[PTBL] = fread8(handle);
        instr[c].ptr[FTBL] = fread8(handle);
        instr[c].ptr[STBL] = fread8(handle);
        instr[c].vibdelay = fread8(handle);
        instr[c].gatetimer = fread8(handle);
        instr[c].firstwave = fread8(handle);
        fread(&instr[c].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (c = 0; c < MAX_TABLES; c++)
      {
        loadsize = fread8(handle);
        tbllen[c] = loadsize;
        fread(ltable[c], loadsize, 1, handle);
        fread(rtable[c], loadsize, 1, handle);
      }
      // Read patterns
      amount = fread8(handle);
      for (c = 0; c < amount; c++)
      {
        length = fread8(handle) * 4;
        fread(pattern[c], length, 1, handle);
      }
      countpatternlengths();
    }

    // Goattracker v2.xx (3-table) import
    if (!memcmp(ident, "GTS2", 4))
    {
      int d;
      int length;
      int amount;
      int loadsize;
      clearsong(1,1,1,1,1);
      ok = 1;

      // Read infotexts
      fread(songname, sizeof songname, 1, handle);
      fread(authorname, sizeof authorname, 1, handle);
      fread(copyrightname, sizeof copyrightname, 1, handle);

      // Read songorderlists
      amount = fread8(handle);
      highestusedsong = amount - 1;
      for (d = 0; d < amount; d++)
      {
        for (c = 0; c < MAX_CHN; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          fread(songorder[d][c], loadsize, 1, handle);
        }
      }

      // Read instruments
      amount = fread8(handle);
      for (c = 1; c <= amount; c++)
      {
        instr[c].ad = fread8(handle);
        instr[c].sr = fread8(handle);
        instr[c].ptr[WTBL] = fread8(handle);
        instr[c].ptr[PTBL] = fread8(handle);
        instr[c].ptr[FTBL] = fread8(handle);
        instr[c].vibdelay = fread8(handle);
        instr[c].ptr[STBL] = makespeedtable(fread8(handle), MST_FINEVIB, 0) + 1;
        instr[c].gatetimer = fread8(handle);
        instr[c].firstwave = fread8(handle);
        fread(&instr[c].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (c = 0; c < MAX_TABLES-1; c++)
      {
        loadsize = fread8(handle);
        tbllen[c] = loadsize;
        fread(ltable[c], loadsize, 1, handle);
        fread(rtable[c], loadsize, 1, handle);
      }
      // Read patterns
      amount = fread8(handle);
      for (c = 0; c < amount; c++)
      {
        int d;
        length = fread8(handle) * 4;
        fread(pattern[c], length, 1, handle);

        // Convert speedtable-requiring commands
        for (d = 0; d < length; d++)
        {
          switch (pattern[c][d*4+2])
          {
            case CMD_FUNKTEMPO:
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_FUNKTEMPO, 0) + 1;
            break;

            case CMD_PORTAUP:
            case CMD_PORTADOWN:
            case CMD_TONEPORTA:
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_PORTAMENTO, 0) + 1;
            break;

            case CMD_VIBRATO:
            pattern[c][d*4+3] = makespeedtable(pattern[c][d*4+3], MST_FINEVIB, 0) + 1;
            break;
          }
        }
      }
      countpatternlengths();
    }
    // Goattracker 1.xx
    if (!memcmp(ident, "GTS!", 4))
    {
      printf("GT1 songs are not supported. Please re-save the song in GT2.\n");
      exit(1);
    }

    // Convert pulsemodulation speed of < v2.4 songs
    if (ident[3] < '4')
    {
      for (c = 0; c < MAX_TABLELEN; c++)
      {
        if ((ltable[PTBL][c] < 0x80) && (rtable[PTBL][c]))
        {
          int speed = ((signed char)rtable[PTBL][c]);
          speed <<= 1;
          if (speed > 127) speed = 127;
          if (speed < -128) speed = -128;
          rtable[PTBL][c] = speed;
        }
      }
    }

    // Convert old legato/nohr parameters
    if (ident[3] < '5')
    {
        for (c = 1; c < MAX_INSTR; c++)
        {
            if (instr[c].firstwave >= 0x80)
            {
                instr[c].gatetimer |= 0x80;
                instr[c].firstwave &= 0x7f;
            }
            if (!instr[c].firstwave) instr[c].gatetimer |= 0x40;
        }
    }
}

void clearsong(int cs, int cp, int ci, int ct, int cn)
{
  int c;

  if (!(cs | cp | ci | ct | cn)) return;

  for (c = 0; c < MAX_CHN; c++)
  {
    int d;
    if (cs)
    {
      for (d = 0; d < MAX_SONGS; d++)
      {
        memset(&songorder[d][c][0], 0, MAX_SONGLEN+2);
        if (!d)
        {
          songorder[d][c][0] = c;
          songorder[d][c][1] = LOOPSONG;
        }
        else
        {
          songorder[d][c][0] = LOOPSONG;
        }
      }
    }
  }
  if (cn)
  {
    memset(songname, 0, sizeof songname);
    memset(authorname, 0, sizeof authorname);
    memset(copyrightname, 0, sizeof copyrightname);
  }
  if (cp)
  {
    for (c = 0; c < MAX_PATT; c++)
      clearpattern(c);
  }
  if (ci)
  {
    for (c = 0; c < MAX_INSTR; c++)
      clearinstr(c);
  }
  if (ct == 1)
  {
    for (c = MAX_TABLES-1; c >= 0; c--)
    {
      memset(ltable[c], 0, MAX_TABLELEN);
      memset(rtable[c], 0, MAX_TABLELEN);
    }
  }
  countpatternlengths();
}

void clearpattern(int p)
{
  int c;

  memset(pattern[p], 0, MAX_PATTROWS*4);
  for (c = 0; c < defaultpatternlength; c++) pattern[p][c*4] = REST;
  for (c = defaultpatternlength; c <= MAX_PATTROWS; c++) pattern[p][c*4] = ENDPATT;
}

void clearinstr(int num)
{
  memset(&instr[num], 0, sizeof(INSTR));
  if (num)
  {
    instr[num].gatetimer = 2;
    instr[num].firstwave = 0x9;
  }
}

void countpatternlengths(void)
{
  int c, d, e;

  highestusedpatt = 0;
  highestusedinstr = 0;
  for (c = 0; c < MAX_PATT; c++)
  {
    for (d = 0; d <= MAX_PATTROWS; d++)
    {
      if (pattern[c][d*4] == ENDPATT) break;
      if ((pattern[c][d*4] != REST) || (pattern[c][d*4+1]) || (pattern[c][d*4+2]) || (pattern[c][d*4+3]))
        highestusedpatt = c;
      if (pattern[c][d*4+1] > highestusedinstr) highestusedinstr = pattern[c][d*4+1];
    }
    pattlen[c] = d;
  }

  for (e = 0; e < MAX_SONGS; e++)
  {
    for (c = 0; c < MAX_CHN; c++)
    {
      for (d = 0; d < MAX_SONGLEN; d++)
      {
        if (songorder[e][c][d] >= LOOPSONG) break;
        if ((songorder[e][c][d] < REPEAT) && (songorder[e][c][d] > highestusedpatt))
          highestusedpatt = songorder[e][c][d];
      }
      songlen[e][c] = d;
    }
  }
}

int makespeedtable(unsigned data, int mode, int makenew)
{
  int c;
  unsigned char l = 0, r = 0;

  if (!data) return -1;

  switch (mode)
  {
    case MST_NOFINEVIB:
    l = (data & 0xf0) >> 4;
    r = (data & 0x0f) << 4;
    break;

    case MST_FINEVIB:
    l = (data & 0x70) >> 4;
    r = ((data & 0x0f) << 4) | ((data & 0x80) >> 4);
    break;

    case MST_FUNKTEMPO:
    l = (data & 0xf0) >> 4;
    r = data & 0x0f;
    break;

    case MST_PORTAMENTO:
    l = (data << 2) >> 8;
    r = (data << 2) & 0xff;
    break;
    
    case MST_RAW:
    r = data & 0xff;
    l = data >> 8;
    break;
  }

  if (makenew == 0)
  {
    for (c = 0; c < MAX_TABLELEN; c++)
    {
      if ((ltable[STBL][c] == l) && (rtable[STBL][c] == r))
        return c;
    }
  }

  for (c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!ltable[STBL][c]) && (!rtable[STBL][c]))
    {
      ltable[STBL][c] = l;
      rtable[STBL][c] = r;
      return c;
    }
  }
  return -1;
}

void printsonginfo(void)
{
    printf("Songs: %d Patterns: %d Instruments: %d\n", highestusedsong+1, highestusedpatt+1, highestusedinstr+1);
}

void clearntsong(void)
{
    memset(ntwavetbl, 0, sizeof ntwavetbl);
    memset(ntnotetbl, 0, sizeof ntnotetbl);
    memset(ntnexttbl, 0, sizeof ntnexttbl);
    memset(ntpulselimittbl, 0, sizeof ntpulselimittbl);
    memset(ntpulsespdtbl, 0, sizeof ntpulsespdtbl);
    memset(ntpulsenexttbl, 0, sizeof ntpulsenexttbl);
    memset(ntfiltlimittbl, 0, sizeof ntfiltlimittbl);
    memset(ntfiltspdtbl, 0, sizeof ntfiltspdtbl);
    memset(ntfiltnexttbl, 0, sizeof ntfiltnexttbl);
    memset(ntpatterns, 0, sizeof ntpatterns);
    memset(nttracks, 0, sizeof nttracks);
    memset(ntcmdad, 0, sizeof ntcmdad);
    memset(ntcmdsr, 0, sizeof ntcmdsr);
    memset(ntcmdwavepos, 0, sizeof ntcmdwavepos);
    memset(ntcmdpulsepos, 0, sizeof ntcmdpulsepos);
    memset(ntcmdfiltpos, 0, sizeof ntcmdfiltpos);
    ntcmdsize = 0;
    ntwavesize = 0;
    ntpulsesize = 0;
    ntfiltsize = 0;
}

void convertsong(void)
{
    int e,c,f;
    unsigned char lastfiltparam = 0;
    unsigned char lastcutoff = 0;
    unsigned char inswavepos[256];
    unsigned char insvibdelay[256];
    unsigned char insvibparam[256];
    unsigned char insntwavepos[256];
    unsigned char insntlegatopos[256];
    unsigned char insntpulsepos[256];
    unsigned char insntfirstwavepos[256];
    unsigned char insntlastwavepos[256];
    unsigned char vibdelay[256];
    unsigned char vibparam[256];
    unsigned char vibntwavepos[256];
    unsigned char slidentwavepos[256];
    unsigned char waveposmap[256];
    unsigned char pulseposmap[256];
    unsigned char filtposmap[256];
    unsigned char instrmap[256];
    unsigned char legatoinstrmap[256];
    unsigned char slidemap[512];
    unsigned char vibmap[256];
    int inswavevibs = 0;
    int vibratos = 0;

    if (highestusedsong > 15)
    {
        printf("More than 16 songs not supported\n");
        exit(1);
    }
    if (highestusedpatt > 126)
    {
        printf("More than 127 patterns not supported\n");
        exit(1);
    }

    getpatttempos();
    getpattbasetrans();

    // Convert trackdata
    for (e = 0; e <= highestusedsong; e++)
    {
        printf("Converting trackdata for song %d\n", e+1);

        int dest = 0;

        for (c = 0; c < MAX_CHN; c++)
        {
            int sp = 0;
            int len = 0;
            unsigned char positionmap[256];

            int trans = 0;
            int lasttrans = -1; // Make sure transpose resets on song loop, even if GT2 song doesn't include it

            ntsongstart[e][c] = dest;

            while (1)
            {
                int rep = 1;
                int patt = 0;

                if (songorder[e][c][sp] >= LOOPSONG)
                    break;
                while (songorder[e][c][sp] >= TRANSDOWN)
                {
                    positionmap[sp] = dest + 1;
                    trans = songorder[e][c][sp++] - TRANSUP;
                }
                while ((songorder[e][c][sp] >= REPEAT) && (songorder[e][c][sp] < TRANSDOWN))
                {
                    positionmap[sp] = dest + 1;
                    rep = songorder[e][c][sp++] - REPEAT + 1;
                }
                patt = songorder[e][c][sp];
                positionmap[sp] = dest + 1;

                if (trans + pattbasetrans[patt] != lasttrans)
                {
                    nttracks[e][dest++] = ((trans + pattbasetrans[patt]) & 0x7f) | 0x80;
                    lasttrans = trans + pattbasetrans[patt];
                }

                while (rep--)
                {
                    nttracks[e][dest++] = patt + 1;
                }
                sp++;
            }
            sp++;
            nttracks[e][dest++] = 0;
            nttracks[e][dest++] = positionmap[songorder[e][c][sp]];
        }
        if (dest > 255)
        {
            printf("Song %d's trackdata does not fit in 255 bytes\n", e+1);
            exit(1);
        }
        ntsongtotallen[e] = dest;
    }

    // Convert instruments
    memset(instrmap, 0, sizeof instrmap);
    memset(legatoinstrmap, 0, sizeof legatoinstrmap);
    memset(waveposmap, 0, sizeof waveposmap);
    memset(pulseposmap, 0, sizeof pulseposmap);
    memset(filtposmap, 0, sizeof filtposmap);
    memset(slidemap, 0, sizeof slidemap);
    memset(vibmap, 0, sizeof vibmap);

    // Create the "stop pulse" optimization step
    ntpulselimittbl[ntpulsesize] = 0;
    ntpulsespdtbl[ntpulsesize] = 0;
    ntpulsenexttbl[ntpulsesize] = 0;
    ntpulsesize++;

    for (e = 1; e <= highestusedinstr; e++)
    {
        int existingwavepos = 0;
        int f;
        int pulseused = 0;

        // If instrument has no wavetable pointer, assume it is not used
        if (!instr[e].ptr[WTBL])
            continue;
        ntcmdad[ntcmdsize] = instr[e].ad;
        ntcmdsr[ntcmdsize] = instr[e].sr;

        for (f = 0; f < inswavevibs; ++f)
        {
            if (inswavepos[f] == instr[e].ptr[WTBL] && instr[e].ptr[STBL] == insvibparam[f] && instr[e].vibdelay == insvibdelay[f])
            {
                existingwavepos = insntwavepos[f];
                insntfirstwavepos[e] = insntwavepos[f];
                insntlastwavepos[e] = insntlegatopos[f];
                break;
            }
        }
        if (existingwavepos == 0)
        {
            int sp = instr[e].ptr[WTBL]-1;
            int waveendpos = 0;
            int legatoendpos = 0;
            existingwavepos = ntwavesize + 1;

            while (sp < 256 && ntwavesize < 256)
            {
                unsigned char wave = ltable[WTBL][sp];
                unsigned char note = rtable[WTBL][sp];
                waveposmap[sp+1] = ntwavesize + 1;
                sp++;

                if (wave < 0xf0 && note == 0x80)
                    printf("Warning: 'keep frequency unchanged' in wavetable is unsupported\n");
                if (wave < 0xf0 && note >= 0x81 && note < 0x8c)
                {
                    printf("Wavetable has octave 0, can not be converted correctly\n");
                    exit(1);
                }
                if (wave >= 0xf0 && wave < 0xff)
                    printf("Warning: wavetable commands are unsupported\n");

                if (wave == 0xff)
                {
                    ntnexttbl[ntwavesize-1] = waveposmap[note];
                    break;
                }
                else if (wave >= 0x10 && wave <= 0x8f)
                {
                    ntwavetbl[ntwavesize] = wave;
                    ntnotetbl[ntwavesize] = note < 0x80 ? note : note-11;
                    ntnexttbl[ntwavesize] = ntwavesize+1+1;
                    legatoendpos = ntwavesize + 1;
                    if (wave & 0x40)
                        pulseused = 1;
                    ntwavesize++;
                }
                else if (wave >= 0xe1 && wave <= 0xef)
                {
                    ntwavetbl[ntwavesize] = wave - 0xe0;
                    ntnotetbl[ntwavesize] = note < 0x80 ? note : note-11;
                    ntnexttbl[ntwavesize] = ntwavesize+1+1;
                    legatoendpos = ntwavesize + 1;
                    ntwavesize++;
                }
                else if (wave < 0x10)
                {
                    ntwavetbl[ntwavesize] = 0xff - wave;
                    ntnotetbl[ntwavesize] = note < 0x80 ? note : note-11;
                    ntnexttbl[ntwavesize] = ntwavesize+1+1;
                    ntwavesize++;
                }
            }

            inswavepos[inswavevibs] = instr[e].ptr[WTBL];
            insvibdelay[inswavevibs] = instr[e].vibdelay;
            insvibparam[inswavevibs] = instr[e].ptr[STBL];
            insntwavepos[inswavevibs] = existingwavepos;
            insntlegatopos[inswavevibs] = legatoendpos;
            insntfirstwavepos[e] = existingwavepos;
            insntlastwavepos[e] = legatoendpos;
            inswavevibs++;

            waveendpos = ntwavesize;

            if (instr[e].vibdelay > 0 && ntwavesize <= 254)
            {
                int existingvibpos = 0;
                for (f = 0; f < vibratos; ++f)
                {
                    if (vibdelay[f] == instr[e].vibdelay && vibparam[f] == instr[e].ptr[STBL])
                    {
                        existingvibpos = vibntwavepos[f];
                        break;
                    }
                }

                if (existingvibpos == 0)
                {
                    int srcpos = instr[e].ptr[STBL]-1;
                    existingvibpos = ntwavesize+1;
                    if (instr[e].vibdelay > 1 && instr[e].vibdelay < 0x70)
                    {
                        ntwavetbl[ntwavesize] = 0x100 - instr[e].vibdelay;
                        ntnotetbl[ntwavesize] = 0;
                        ntnexttbl[ntwavesize] = ntwavesize+1+1;
                        ntwavesize++;
                    }
                    vibmap[instr[e].ptr[STBL]] = ntwavesize+1;
                    ntwavetbl[ntwavesize] = 0;
                    ntnotetbl[ntwavesize] = 0xff - ltable[STBL][srcpos];
                    ntnexttbl[ntwavesize] = rtable[STBL][srcpos];
                    ntwavesize++;
                    vibdelay[vibratos] = instr[e].vibdelay;
                    vibparam[vibratos] = instr[e].ptr[STBL];
                    vibntwavepos[vibratos] = existingvibpos;
                    vibratos++;
                }
                ntnexttbl[waveendpos-1] = existingvibpos;
            }
        }

        ntcmdwavepos[ntcmdsize] = existingwavepos;

        if (!instr[e].ptr[PTBL])
        {
            if (pulseused)
                ntcmdpulsepos[ntcmdsize] = 0; // Keep existing pulse going
            else
                ntcmdpulsepos[ntcmdsize] = 1; // Stop pulse-step, for saving rastertime for triangle/sawtooth/noise only instruments
        }
        else
        {
            int existingpulsepos = pulseposmap[instr[e].ptr[PTBL]];

            if (existingpulsepos == 0)
            {
                int sp = instr[e].ptr[PTBL]-1;
                int pulsevalue = 0;

                while (sp < 256 && ntpulsesize < 128)
                {
                    unsigned char time = ltable[PTBL][sp];
                    unsigned char spd = rtable[PTBL][sp];
                    pulseposmap[sp+1] = (ntpulsesize + 1) | (time & 0x80);
                    sp++;

                    if (time != 0xff && spd & 0xf)
                    {
                        printf("Warning: lowest 4 bits of pulse aren't supported\n");
                    }
                    
                    if (time == 0xff)
                    {
                        ntpulsenexttbl[ntpulsesize-1] = pulseposmap[spd];
                        break;
                    }
                    else if (time & 0x80)
                    {
                        ntpulselimittbl[ntpulsesize] = (time & 0xf) | (spd & 0xf0);
                        ntpulsespdtbl[ntpulsesize] = 0;
                        ntpulsenexttbl[ntpulsesize] = ntpulsesize+1+1;
                        if (ntpulsesize > 1 && ntpulsenexttbl[ntpulsesize-1] == ntpulsesize+1)
                            ntpulsenexttbl[ntpulsesize-1] |= 0x80;
                        pulsevalue = ((time & 0xf) << 8) | spd;
                    }
                    else
                    {
                        if (spd < 0x80)
                            pulsevalue += time*spd;
                        else
                            pulsevalue += time*((int)spd-0x100);
                        ntpulselimittbl[ntpulsesize] = (pulsevalue >> 8) | (pulsevalue & 0xf0);
                        if (spd < 0x80)
                            ntpulsespdtbl[ntpulsesize] = spd & 0xf0;
                        else
                            ntpulsespdtbl[ntpulsesize] = (spd & 0xf0) - 1;
                        ntpulsenexttbl[ntpulsesize] = ntpulsesize+1+1;
                    }

                    ++ntpulsesize;
                }
                existingpulsepos = pulseposmap[instr[e].ptr[PTBL]];
            }
            
            ntcmdpulsepos[ntcmdsize] = existingpulsepos;
        }
        
        if (!instr[e].ptr[FTBL])
        {
            ntcmdfiltpos[ntcmdsize] = 0;
        }
        else
        {
            int existingfiltpos = filtposmap[instr[e].ptr[FTBL]];

            if (existingfiltpos == 0)
            {
                int sp = instr[e].ptr[FTBL]-1;
                unsigned char cutoffvalue = 0;

                while (sp < 256 && ntfiltsize < 128)
                {
                    unsigned char time = ltable[FTBL][sp];
                    unsigned char spd = rtable[FTBL][sp];
                    filtposmap[sp+1] = (ntfiltsize + 1) | (time & 0x80);
                    sp++;

                    if (time == 0xff && ntfiltsize > 0)
                    {
                        ntfiltnexttbl[ntfiltsize-1] = filtposmap[spd];
                        break;
                    }
                    else if (time & 0x80)
                    {
                        if (time & 0x40)
                            printf("Warning: highpass filter not supported\n");
                        ntfiltspdtbl[ntfiltsize] = (time & 0x30) | (spd & 0xcf);
                        ntfiltnexttbl[ntfiltsize] = ntfiltsize+1+1;
                        if (ltable[FTBL][sp] != 0)
                            printf("Warning: filter init-step not followed by set cutoff-step\n");
                        if (ntfiltsize > 1 && ntfiltnexttbl[ntfiltsize-1] == ntfiltsize+1)
                            ntfiltnexttbl[ntfiltsize-1] |= 0x80;
                        ++ntfiltsize;
                    }
                    else if (time == 0 && ntfiltsize > 0)
                    {
                        // Fill in the initial cutoff value of the init step above
                        ntfiltlimittbl[ntfiltsize-1] = spd;
                        cutoffvalue = spd;
                    }
                    else
                    {
                        if (spd < 0x80)
                            cutoffvalue += time*spd;
                        else
                            cutoffvalue += time*((int)spd-0x100);
                        ntfiltlimittbl[ntfiltsize] = cutoffvalue;
                        ntfiltspdtbl[ntfiltsize] = spd;
                        ntfiltnexttbl[ntfiltsize] = ntfiltsize+1+1;
                        ++ntfiltsize;
                    }
                }
                existingfiltpos = filtposmap[instr[e].ptr[FTBL]];
            }
            
            ntcmdfiltpos[ntcmdsize] = existingfiltpos;
        }

        instrmap[e] = ntcmdsize+1;
        ntcmdsize++;
    }

    // Convert patterns
    printf("Converting patterns\n");

    for (e = 0; e <= highestusedpatt; e++)
    {
        unsigned char notecolumn[MAX_PATTROWS+1];
        unsigned char cmdcolumn[MAX_PATTROWS+1];
        unsigned char durcolumn[MAX_PATTROWS+1];
        memset(cmdcolumn, 0, sizeof cmdcolumn);
        memset(durcolumn, 0, sizeof durcolumn);
        int pattlen = 0;
        int lastnoteins = -1;
        int lastnotentins = -1;
        int lastdur;
        int lastwaveptr = 0;
        int d = 0;
        unsigned short freq = 0;
        int targetfreq = 0;
        int tptargetnote = 0;
        int tpstepsleft = 0;

        for (c = 0; c < MAX_PATTROWS+1; c++)
        {
            int note = pattern[e][c*4];
            int gtcmd = pattern[e][c*4+2];
            int gtcmddata = pattern[e][c*4+3];
            if (note == ENDPATT)
            {
                notecolumn[d] = NT_ENDPATT;
                break;
            }
            int instr = pattinstr[e][c];
            int ntinstr = instrmap[instr];
            int dur = patttempo[e][c];
            int waveptr = 0;

            // If tempo not determined, use default 6
            if (!dur)
                dur = 6;

            // If is not a toneportamento with speed, reset frequency before processing effects
            if (note >= FIRSTNOTE+12 && note <= LASTNOTE)
            {
                if (gtcmd != 0x3 || gtcmddata == 0)
                {
                    freq = freqtbl[note+pattbasetrans[e]-FIRSTNOTE-12];
                }
            }

            if (gtcmd != 0)
            {
                if (gtcmd == 0xf)
                {
                    // No-op, tempo already accounted for
                }
                else if (gtcmd == 0x3)
                {
                    if (gtcmddata == 0 && note >= FIRSTNOTE+12 && note <= LASTNOTE) // Legato note start
                    {
                        if (legatoinstrmap[instr])
                            ntinstr = legatoinstrmap[instr] + 0x80;
                        else
                        {
                            legatoinstrmap[instr] = ntcmdsize+1;
                            ntcmdad[ntcmdsize] = 0;
                            ntcmdsr[ntcmdsize] = 0;
                            ntcmdwavepos[ntcmdsize] = insntlastwavepos[instr];
                            ntcmdpulsepos[ntcmdsize] = 0;
                            ntcmdfiltpos[ntcmdsize] = 0;
                            ntcmdsize++;
                            ntinstr = legatoinstrmap[instr] + 0x80;
                        }
                    }
                    else if (gtcmddata > 0 && note >= FIRSTNOTE+12 && note <= LASTNOTE)
                    {
                        targetfreq = freqtbl[note+pattbasetrans[e]-FIRSTNOTE-12];
                        unsigned short speed = 0;
                        if (gtcmddata != 0)
                            speed = (ltable[STBL][gtcmddata-1] << 8) | rtable[STBL][gtcmddata-1];
                        tpstepsleft = speed > 0 ? abs(freq-targetfreq) / speed : MAX_PATTROWS;
                        tpstepsleft += 1; // Assume 1 step gets lost

                        if (targetfreq > freq)
                        {
                            if (gtcmddata != 0 && slidemap[gtcmddata] == 0 && ntwavesize < 256)
                            {
                                ntwavetbl[ntwavesize] = 0x90;
                                ntnexttbl[ntwavesize] = (speed-1) >> 8;
                                ntnotetbl[ntwavesize] = (speed-1) & 0xff;
                                slidemap[gtcmddata] = ntwavesize+1;
                                ntwavesize++;
                            }
                            waveptr = slidemap[gtcmddata];
                        }
                        else
                        {
                            speed = -speed;
                            if (gtcmddata != 0 && slidemap[gtcmddata+0x100] == 0 && ntwavesize < 256)
                            {
                                ntwavetbl[ntwavesize] = 0x90;
                                ntnexttbl[ntwavesize] = (speed-1) >> 8;
                                ntnotetbl[ntwavesize] = (speed-1) & 0xff;
                                slidemap[gtcmddata+0x100] = ntwavesize+1;
                                ntwavesize++;
                           }
                            waveptr = slidemap[gtcmddata+0x100];
                        }
                        tptargetnote = note;
                        note = REST; // The actual toneportamento target note is just discarded, as there is no "stop at note" functionality in the player

                    }
                }
                else if (gtcmd == 0x1)
                {
                    unsigned short speed = 0;
                    if (gtcmddata != 0)
                        speed = (ltable[STBL][gtcmddata-1] << 8) | rtable[STBL][gtcmddata-1];

                    if (gtcmddata != 0 && slidemap[gtcmddata] == 0 && ntwavesize < 256)
                    {
                        ntwavetbl[ntwavesize] = 0x90;
                        ntnexttbl[ntwavesize] = (speed-1) >> 8;
                        ntnotetbl[ntwavesize] = (speed-1) & 0xff;
                        slidemap[gtcmddata] = ntwavesize+1;
                        ntwavesize++;
                    }
                    waveptr = slidemap[gtcmddata];
                    freq += (dur-3) * speed; // Assume 3 steps get lost for toneportamento purposes
                }
                else if (gtcmd == 0x2)
                {
                    unsigned short speed = 0;
                    if (gtcmddata != 0)
                        speed = -((ltable[STBL][gtcmddata-1] << 8) | rtable[STBL][gtcmddata-1]);

                    if (gtcmddata != 0 && slidemap[gtcmddata+0x100] == 0 && ntwavesize < 256)
                    {
                        ntwavetbl[ntwavesize] = 0x90;
                        ntnexttbl[ntwavesize] = (speed-1) >> 8;
                        ntnotetbl[ntwavesize] = (speed-1) & 0xff;
                        slidemap[gtcmddata+0x100] = ntwavesize+1;
                        ntwavesize++;
                    }
                    waveptr = slidemap[gtcmddata+0x100];
                    freq += (dur-3) * speed; // Assume 3 steps get lost for toneportamento purposes
                }
                else if (gtcmd == 0x4)
                {
                    if (gtcmddata != 0 && vibmap[gtcmddata] == 0 && ntwavesize < 256)
                    {
                        int srcpos = gtcmddata-1;
                        ntwavetbl[ntwavesize] = 0;
                        ntnotetbl[ntwavesize] = 0xff - ltable[STBL][srcpos];
                        ntnexttbl[ntwavesize] = rtable[STBL][srcpos];
                        vibmap[gtcmddata] = ntwavesize+1;
                        ntwavesize++;
                    }
                    waveptr = vibmap[gtcmddata];
                }
                else
                {
                    printf("Warning: unsupported command %x in pattern %d step %d\n", gtcmd, e, c);
                }
            }

            durcolumn[d] = dur;

            // Check if toneportamento ends now
            if (tptargetnote)
            {
                if (tpstepsleft < dur)
                {
                    if (legatoinstrmap[lastnoteins])
                        ntinstr = legatoinstrmap[lastnoteins] + 0x80;
                    else
                    {
                        legatoinstrmap[lastnoteins] = ntcmdsize+1;
                        ntcmdad[ntcmdsize] = 0;
                        ntcmdsr[ntcmdsize] = 0;
                        ntcmdwavepos[ntcmdsize] = insntlastwavepos[lastnoteins];
                        ntcmdpulsepos[ntcmdsize] = 0;
                        ntcmdfiltpos[ntcmdsize] = 0;
                        ntcmdsize++;
                        ntinstr = legatoinstrmap[instr] + 0x80;
                    }

                    if (tpstepsleft == 0)
                    {
                        notecolumn[d] = (tptargetnote-pattbasetrans[e]-FIRSTNOTE-12)*2+NT_FIRSTNOTE;
                        cmdcolumn[d] = ntinstr;
                    }
                    else
                    {
                        if (waveptr && waveptr != lastwaveptr)
                        {
                            notecolumn[d] = NT_WAVEPTR;
                            cmdcolumn[d] = waveptr;
                            durcolumn[d] = tpstepsleft;
                        }
                        else
                        {
                            notecolumn[d] = NT_REST;
                            cmdcolumn[d] = 0;
                            durcolumn[d] = tpstepsleft;
                        }
                        ++d;
                        notecolumn[d] = (tptargetnote-pattbasetrans[e]-FIRSTNOTE-12)*2+NT_FIRSTNOTE;
                        cmdcolumn[d] = ntinstr;
                        durcolumn[d] = dur-tpstepsleft;
                    }
                    ++d;
                    tptargetnote = 0;
                    lastwaveptr = 0; // TP ended, consider next waveptr command individually again
                    lastnotentins = ntinstr;
                    continue;
                }
                else
                {
                    tpstepsleft -= dur;
                }
            }

            if (note >= FIRSTNOTE+12 && note <= LASTNOTE)
            {
                lastwaveptr = 0; // If triggering a note, forget the last waveptr

                notecolumn[d] = (note-pattbasetrans[e]-FIRSTNOTE-12)*2+NT_FIRSTNOTE;
                if (ntinstr != lastnotentins)
                {
                    cmdcolumn[d] = ntinstr;
                    lastnoteins = instr;
                    lastnotentins = ntinstr;
                }
                // If waveptr in combination with note, must split step
                if (waveptr && waveptr != lastwaveptr)
                {
                    int requireddur = insntlastwavepos[instr]-insntfirstwavepos[instr] + 3;
                    if (dur > requireddur)
                    {
                        durcolumn[d] = requireddur;
                        ++d;
                        notecolumn[d] = NT_WAVEPTR;
                        cmdcolumn[d] = waveptr;
                        durcolumn[d] = dur-requireddur;
                    }
                }
            }
            else
            {
                notecolumn[d] = note == KEYOFF ? NT_KEYOFF : NT_REST;
                if (waveptr && waveptr != lastwaveptr)
                {
                    notecolumn[d] = NT_WAVEPTR;
                    cmdcolumn[d] = waveptr;
                }
            }

            ++d;
            lastwaveptr = waveptr;
        }

        for (c = 0; c < MAX_PATTROWS+1;)
        {
            int merge = 0;
            if (notecolumn[c+1] != NT_ENDPATT)
            {
                if ((durcolumn[c] + durcolumn[c+1]) <= maxdur && cmdcolumn[c+1] == 0)
                {
                    if (notecolumn[c] == NT_KEYOFF && notecolumn[c+1] == NT_KEYOFF)
                        merge = 1;
                    else if (notecolumn[c] == NT_REST && notecolumn[c+1] == NT_REST)
                        merge = 1;
                    else if (notecolumn[c] == NT_KEYOFF && notecolumn[c+1] == NT_REST)
                        merge = 1;
                    else if (notecolumn[c] == NT_WAVEPTR && notecolumn[c+1] == NT_REST)
                        merge = 1;
                    else if (notecolumn[c] >= NT_FIRSTNOTE && notecolumn[c] <= NT_LASTNOTE && notecolumn[c+1] == NT_REST)
                        merge = 1;
                }
            }

            if (merge)
            {
                int d;
                durcolumn[c] += durcolumn[c+1];
                for (d = c+1; d < MAX_PATTROWS; d++)
                {
                    notecolumn[d] = notecolumn[d+1];
                    cmdcolumn[d] = cmdcolumn[d+1];
                    durcolumn[d] = durcolumn[d+1];
                }
            }
            else
                c++;
        }

        // Check if two consecutive durations can be averaged
        for (c = 0; c < MAX_PATTROWS+1; c++)
        {
            if (notecolumn[c+1] != NT_ENDPATT)
            {
                int sum = durcolumn[c] + durcolumn[c+1];
                int average = 0;
                if (!(sum & 1) && durcolumn[c] != durcolumn[c+1] && cmdcolumn[c+1] == 0 && (notecolumn[c+2] == NT_ENDPATT || durcolumn[c+2] != durcolumn[c+1]))
                {
                    if (notecolumn[c] == NT_KEYOFF && notecolumn[c+1] == NT_KEYOFF)
                        average = 1;
                    else if (notecolumn[c] == NT_REST && notecolumn[c+1] == NT_REST)
                        average = 1;
                    else if (notecolumn[c] == NT_KEYOFF && notecolumn[c+1] == NT_REST)
                        average = 1;
                    else if (notecolumn[c] == NT_WAVEPTR && notecolumn[c+1] == NT_REST)
                        average = 1;
                    else if (notecolumn[c] >= NT_FIRSTNOTE && notecolumn[c] <= NT_LASTNOTE && notecolumn[c+1] == NT_REST)
                        average = 1;
                }
                if (average == 1)
                {
                    durcolumn[c] = durcolumn[c+1] = sum/2;
                    c++;
                }
            }
        }
        
        // Remove 1-2-1 duration changes if possible
        for (c = 1; c < MAX_PATTROWS+1; c++)
        {
            if (notecolumn[c] == NT_ENDPATT || notecolumn[c+1] == NT_ENDPATT)
                break;
            if (durcolumn[c+1] == durcolumn[c-1] && durcolumn[c] == 2*durcolumn[c-1] && notecolumn[c] >= NT_FIRSTNOTE && notecolumn[c] <= NT_LASTNOTE)
            {
                int d;
                for (d = MAX_PATTROWS - 1; d > c; d--)
                {
                    notecolumn[d+1] = notecolumn[d];
                    cmdcolumn[d+1] = cmdcolumn[d];
                    durcolumn[d+1]= durcolumn[d];
                }
                notecolumn[c+1] = NT_REST;
                cmdcolumn[c+1] = 0;
                durcolumn[c] = durcolumn[c+1] = durcolumn[c-1];
            }
        }

        // Fix if has too short durations
        for (c = 0; c < MAX_PATTROWS; c++)
        {
            if (notecolumn[c] == NT_ENDPATT)
                break;
            if (durcolumn[c] > 0 && durcolumn[c] < 3)
            {
                int orig = durcolumn[c];
                int inc = 3-durcolumn[c];
                durcolumn[c] += inc;
                durcolumn[c+1] -= inc;
                printf("Warning: adjusting too short duration in pattern %d step %d to %d (next step adjusted to %d)\n", e, c, durcolumn[c], durcolumn[c+1]);
            }
        }
        
        // Clear unneeded durations
        for (c = 0; c < MAX_PATTROWS+1; c++)
        {
            if (notecolumn[c] == NT_ENDPATT)
                break;
            if (c && durcolumn[c] == lastdur)
                durcolumn[c] = 0;
            if (durcolumn[c])
                lastdur = durcolumn[c];
        }

        // Build the final patterndata
        for (c = 0; c < MAX_PATTROWS+1; c++)
        {
            if (notecolumn[c] == NT_ENDPATT)
            {
                ntpatterns[e][pattlen++] = NT_ENDPATT;
                break;
            }

            if (notecolumn[c] >= NT_FIRSTNOTE && notecolumn[c] <= NT_LASTNOTE)
            {
                if (cmdcolumn[c])
                {
                    ntpatterns[e][pattlen++] = notecolumn[c];
                    ntpatterns[e][pattlen++] = cmdcolumn[c];
                }
                else
                    ntpatterns[e][pattlen++] = notecolumn[c] + NT_NOCMD;
            }
            else if (notecolumn[c] == NT_WAVEPTR)
            {
                ntpatterns[e][pattlen++] = notecolumn[c];
                ntpatterns[e][pattlen++] = cmdcolumn[c];
            }
            else
            {
                ntpatterns[e][pattlen++] = notecolumn[c];
            }
            if (durcolumn[c])
            {
                ntpatterns[e][pattlen++] = 0x101 - durcolumn[c];
            }
        }

        if (pattlen > MAX_NTPATTLEN)
        {
            printf("Pattern %d does not fit in 256 bytes when compressed\n", e);
            exit(1);
        }
        ntpattlen[e] = pattlen;
    }
}

void getpattbasetrans(void)
{
    int c,e;
    memset(pattbasetrans, 0, sizeof pattbasetrans);

    for (e = 0; e <= highestusedpatt; e++)
    {
        int basetrans = 0;
        for (c = 0; c < MAX_PATTROWS+1; c++)
        {
            int note = pattern[e][c*4];
            if (note >= FIRSTNOTE && note < FIRSTNOTE+12)
            {
                printf("Pattern %d: octave 0 is not supported\n", e);
                exit(1);
            }
            if (note >= FIRSTNOTE && note <= LASTNOTE)
            {
                if (note >= FIRSTNOTE+6*12)
                {
                    while (note - basetrans >= FIRSTNOTE+6*12)
                        basetrans += 12;
                }
            }
        }
        for (c = 0; c < MAX_PATTROWS+1; c++)
        {
            int note = pattern[e][c*4];
            if (note >= FIRSTNOTE && note <= LASTNOTE)
            {
                if (note - basetrans < FIRSTNOTE+12)
                {
                    printf("Pattern %d has too wide note range\n", e);
                    exit(1);
                }
            }
        }
        if (basetrans > 0)
            printf("Pattern %d basetranspose %d\n", e, basetrans);
        pattbasetrans[e] = basetrans;
    }
}
void getpatttempos(void)
{
    int e,c;

    memset(patttempo, 0, sizeof patttempo);
    memset(pattinstr, 0, sizeof pattinstr);

    // Simulates playroutine going through the songs
    for (e = 0; e <= highestusedsong; e++)
    {
        printf("Determining tempo & instruments for song %d\n", e+1);
        int sp[3] = {-1,-1,-1};
        int pp[3] = {0xff,0xff,0xff};
        int pn[3] = {0,0,0};
        int rep[3] = {0,0,0};
        int stop[3] = {0,0,0};
        int instr[3] = {1,1,1};
        int tempo[3] = {6,6,6};
        int tick[3] = {0,0,0};
        int keyon[3] = {0,0,0};

        while (!stop[0] || !stop[1] || !stop[2])
        {
            for (c = 0; c < MAX_CHN; c++)
            {
                if (!stop[c] && !tick[c])
                {
                    if (pp[c] == 0xff || pattern[pn[c]][pp[c]*4] == ENDPATT)
                    {
                        if (!rep[c])
                        {
                            sp[c]++;
                            if (songorder[e][c][sp[c]] >= LOOPSONG)
                            {
                                stop[c] = 1;
                                break;
                            }
                            while (songorder[e][c][sp[c]] >= TRANSDOWN)
                                sp[c]++;
                            while ((songorder[e][c][sp[c]] >= REPEAT) && (songorder[e][c][sp[c]] < TRANSDOWN))
                            {
                                rep[c] = songorder[e][c][sp[c]] - REPEAT;
                                sp[c]++;
                            }
                        }
                        else
                            rep[c]--;

                        pn[c] = songorder[e][c][sp[c]];
                        pp[c] = 0;
                    }

                    if (!stop[c])
                    {
                        int note = pattern[pn[c]][pp[c]*4];
                        if (note >= FIRSTNOTE && note <= LASTNOTE && pattern[pn[c]][pp[c]*4+2] != CMD_TONEPORTA)
                            keyon[c] = 1;
                        if (note == KEYON)
                            keyon[c] = 1;
                        if (note == KEYOFF)
                            keyon[c] = 0;

                        instr[c] = pattern[pn[c]][pp[c]*4+1];

                        if (pattern[pn[c]][pp[c]*4+2] == 0xf)
                        {
                            int newtempo = pattern[pn[c]][pp[c]*4+3];
                            if (newtempo < 0x80)
                            {
                                tempo[0] = newtempo;
                                tempo[1] = newtempo;
                                tempo[2] = newtempo;
                            }
                            else
                                tempo[c] = newtempo & 0x7f;
                        }
                    }
                }
            }

            for (c = 0; c < MAX_CHN; c++)
            {
                if (!stop[c])
                {
                    if (patttempo[pn[c]][pp[c]] != 0 && patttempo[pn[c]][pp[c]] != tempo[c])
                    {
                        // We have detected pattern playback with multiple tempos
                        int f;
                        int pattfound = 0;

                        for (f = 0; f < remappedpatterns; f++)
                        {
                            if (pattremaptempo[f] == tempo[c] && pattremapsrc[f] == pn[c])
                            {
                                pattfound = 1;
                                pn[c] = pattremapdest[f];
                                songorder[e][c][sp[c]] = pn[c];
                                break;
                            }
                        }

                        if (!pattfound)
                        {
                            if (highestusedpatt >= MAX_PATT-1)
                            {
                                printf("Not enough patterns free to perform tempo remapping");
                                exit(1);
                            }
                            printf("Remapping pattern %d to %d as it's played at multiple tempos\n", pn[c], highestusedpatt+1);
                            memcpy(&pattern[highestusedpatt+1][0], &pattern[pn[c]][0], MAX_PATTROWS*4+4);
                            pattremaptempo[remappedpatterns] = tempo[c];
                            pattremapsrc[remappedpatterns] = pn[c];
                            pattremapdest[remappedpatterns] = highestusedpatt+1;
                            remappedpatterns++;
                            pn[c] = highestusedpatt+1;
                            songorder[e][c][sp[c]] = pn[c];
                            highestusedpatt++;
                        }
                    }
                    
                    if (!tick[c])
                    {
                        patttempo[pn[c]][pp[c]] = tempo[c];
                        pattinstr[pn[c]][pp[c]] = instr[c];
                        pattkeyon[pn[c]][pp[c]] = keyon[c];
                    }

                    tick[c]++;
                    if (tick[c] >= tempo[c])
                    {
                        tick[c] = 0;
                        pp[c]++;
                    }
                }
            }
        }
    }
}

void saventsong(const char* songfilename)
{
    int c;

    out = fopen(songfilename, "wt");
    if (!out)
    {
        printf("Could not open destination file %s\n", songfilename);
        exit(1);
    }

    fprintf(out, "songTbl:\n");
    for (c = 0; c <= highestusedsong; ++c)
    {
        fprintf(out, "  dc.w song%d-1\n", c);
        fprintf(out, "  dc.b $%02x\n", ntsongstart[c][0]+1);
        fprintf(out, "  dc.b $%02x\n", ntsongstart[c][1]+1);
        fprintf(out, "  dc.b $%02x\n", ntsongstart[c][2]+1);
    }
    fprintf(out, "\n");

    for (c = 0; c <= highestusedsong; ++c)
    {
        char namebuf[80];
        sprintf(namebuf, "song%d", c);
        writeblock(out, namebuf, &nttracks[c][0], ntsongtotallen[c]);
    }

    fprintf(out, "pattTblLo:\n");
    for (c = 0; c <= highestusedpatt; ++c)
        fprintf(out, "  dc.b <patt%d\n", c);
    fprintf(out, "\n");
    fprintf(out, "pattTblHi:\n");
    for (c = 0; c <= highestusedpatt; ++c)
        fprintf(out, "  dc.b >patt%d\n", c);
    fprintf(out, "\n");

    for (c = 0; c <= highestusedpatt; ++c)
    {
        char namebuf[80];
        sprintf(namebuf, "patt%d", c);
        writeblock(out, namebuf, &ntpatterns[c][0], ntpattlen[c]);
    }
    
    writeblock(out, "insAD", ntcmdad, ntcmdsize);
    writeblock(out, "insSR", ntcmdsr, ntcmdsize);
    writeblock(out, "insWavePos", ntcmdwavepos, ntcmdsize);
    writeblock(out, "insPulsePos", ntcmdpulsepos, ntcmdsize);
    writeblock(out, "insFiltPos", ntcmdfiltpos, ntcmdsize);
    writeblock(out, "waveTbl", ntwavetbl, ntwavesize);
    writeblock(out, "noteTbl", ntnotetbl, ntwavesize);
    writeblock(out, "waveNextTbl", ntnexttbl, ntwavesize);
    writeblock(out, "pulseLimitTbl", ntpulselimittbl, ntpulsesize);
    writeblock(out, "pulseSpdTbl", ntpulsespdtbl, ntpulsesize);
    writeblock(out, "pulseNextTbl", ntpulsenexttbl, ntpulsesize);
    writeblock(out, "filtLimitTbl", ntfiltlimittbl, ntfiltsize);
    writeblock(out, "filtSpdTbl", ntfiltspdtbl, ntfiltsize);
    writeblock(out, "filtNextTbl", ntfiltnexttbl, ntfiltsize);
}

void writeblock(FILE* out, const char* name, unsigned char* data, int len)
{
    int c;
    fprintf(out, "%s:\n", name);
    for (c = 0; c < len; ++c)
    {
        if (c == 0)
            fprintf(out, "  dc.b $%02x", data[c]);
        else if ((c & 7) == 0)
            fprintf(out, "\n  dc.b $%02x", data[c]);
        else
            fprintf(out, ",$%02x", data[c]);
    }
    fprintf(out, "\n\n");
}