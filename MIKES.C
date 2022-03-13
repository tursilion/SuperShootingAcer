/* Mike's subroutines for use with the Personal C Compiler */
/* added to here and there, as needed */

extern int scr_mode;  /* extern to PCIOs mode variable */
extern unsigned _rax,_rbx; /* interrupt access registers */

unsigned int speed; /* holds value used by delay for 1/60 second */

int line_table[200]; /* holds offsets of 200 screen lines */
int scrn_base;   /* holds segment base of screen */
int scrn_lines;  /* holds max line # on the screen (normally 199) */
int NO_BLACK;

char pal[768];  /* holds a VGA palette. Wasted space under CGA */

mike_init()
{ /* initialize whatever needs to be initialized here */
set_speed();
scr_setup();  /* must link with PCIO! */
}

set_speed()
{ /* set the variable "speed" based on the machine */
char x1[10],x2[10];
unsigned int q;

q=0;                   /* counter */
times(x1);             /* get current time */
times(x2);             /* get it again (HHMMSS) */
while (x2[7]==x1[7])   /* compare seconds */
 times(x1);            /* get new time */
times(x2);             /* now we're starting at the beginning of a second */
while (x2[7]==x1[7])   /* wait for the seconds to change again */
{ q++; times(x2); }    /* while counting and getting new time */
speed=q/60;            /* q loops in 1 second. Divide by 60 for jiffies */
}

delay(x) unsigned int x;
{ /* pauses */
unsigned int a;  char x2[10];

for (a=0; a<x; a++)    /* simple delay loop, set up like the loop in */
 times(x2);            /* set_speed. delay(speed) will pause approx */
}                      /* 1/60th of a second */

sound_on(freq_cnt) int freq_cnt;
{ /* start sounds. Freq_cnt = 1,193,180/hz */
_outb(182,67); /* get ready for sound data */
_outb(freq_cnt%256,66); /* low byte */
_outb(freq_cnt/256,66); /* high byte */
_outb(_inb(97)|3,97); /* start sound */
}

sound_off()
{ /* turn sound off */
_outb(_inb(97)&252,97);
}

play(note,dur) int note, dur;
{ /* play note(in Hz) for dur/60 seconds. needs SPEED to be set up
     cheats by diving Hz by 20, so the formula for code becomes
     59659/(hz/20), which can be done in 16 bits */

note=note/20;
sound_on(59659/note);
delay(speed*dur);
sound_off();
}

set_txt()
{ /* set text mode */
scr_setmode(3);
}

set_cga()
{ /* set CGA mono mode (6), build screen table, set segment variable */
int i;

scr_setmode(6);
for (i=0; i<200; ++i) 
  line_table[i]=(0x2000*(i%2))+(80*(i/2));
scrn_base=0xb800;
scrn_lines=199;
}

/* EGA isn't really worth the effort here. It requires extra processing
for everything it does. VGA colour, or CGA mono. That's it. */

set_vga()
{ /* set VGA 256-colour mode, build table, set base variable */
int i;

scr_setmode(19);
for (i=0; i<200; ++i) 
  line_table[i]=i*320;
scrn_base=0xa000;
scrn_lines=199;
}

put(data,x,y) char data[]; int x,y;
{ /* puts the bitmapped image in "data" at coordinates x,y.
     Takes screen mode into account, should use assembly. Assumes
     that data is meant for the mode in use (pre-converted).
     VGA should be in one-byte per pixel, and CGA should be 
     in monochrome format. CGA will not be shifted, only adjusted. 
     If better is required, it'd only slow things down. 
     Buy a VGA monitor. */
int nr,nc,i1,i3,ds;

nr=(char)data[0];
nc=(char)data[1];
i3=2;

if (y+nr>scrn_lines) nr=scrn_lines-y+1;  /* clip bottom */
if (y<0) { i3=2+nc*(0-y); nr=nr+y; y=0; }         /* clip top */
switch(scr_mode)
{ case 6 : /* CGA mono */
           x=x >> 2;  /* *2/8, or /4 for byte offset */
  case 19: /* VGA 256c */
           /* both are handled the same way, because of the data format */ 
           /* this is SCREAMING for assembly... */
           ds=_showds();   /* only call this once */
           for (i1=0; i1<nr; i1++)
           /* long data move. _showds() gives the C data segment address */
           { _lmove(nc,&data[i3],ds,line_table[y+i1]+x,scrn_base);
             i3=i3+nc; }
}
}

palette(p,n) char p[]; int n;
{ /* set up to n colours from the palette and store in pal. For VGA only, 3
     bytes per entry, 256 max entries (max size, 768 bytes) */
int i,i2;

     if (scr_mode!=19) return(0);  /* no palette unless VGA */
     if ((n<1)||(n>256)) return(0); /* don't set bad number */
     i2=0;
     _outb(0xff,0x3c6);   /* setup <?> */
     for (i=0; i<n; i++)
     { _outb(i,0x3c8);   /* palette index number */
       _outb(pal[i2]=(char)p[i2++],0x3c9);  /* red */
       _outb(pal[i2]=(char)p[i2++],0x3c9);  /* green */
       _outb(pal[i2]=(char)p[i2++],0x3c9);  /* blue */
     }
}

border(n) int n;
{ /* set VGA border colour */
if (scr_mode!=19) return(0); /* only if in VGA mode */
_rax=0x1001;
_rbx=n*256;
_doint(0x10);
}

get(data,x,y,nr,nc) char data[]; int x,y,nr,nc;
{ /* get a bitmap from the current screen. Slight range checking */
int i1,i3,ds;

data[0]=nr;
i3=2;
switch(scr_mode)
{ case 6 : x=x >> 2; /* CGA mono offset */
           nc=nc >> 2;
  case 19: data[1]=nc;
           if (y+nr>scrn_lines) nr=scrn_lines-y+1;  /* clip bottom */
           if (y<0) { i3=2+nc*(0-y); nr=nr+y; y=0; }         /* clip top */
           ds=_showds();  
           for (i1=0; i1<nr; i1++)
           { _lmove(nc,line_table[y+i1]+x,scrn_base,&data[i3],ds);  
             i3=i3+nc; }
}
}

colour(i,r,g,b) int i,r,g,b;
{ /* set one VGA colour. I[ndex] 0-255, R,G,B each 0-63 */

if (scr_mode!=19) return(0);

_outb(0xff,0x3c6);
_outb(i,0x3c8);
_outb(r,0x3c9);
_outb(g,0x3c9);
_outb(b,0x3c9);
pal[i*3]=(char)r;
pal[i*3+1]=(char)g;
pal[i*3+2]=(char)b;
}

putm(data,x,y) char data[]; int x,y;
{ /* puts the bitmapped image in "data" at coordinates x,y.
     Uses a mask to preserve background. In VGA, colour 0
     is ignored. In CGA, every two bytes of data are preceded by
     a mask byte. Only pixels corresponding to '1' pixels in
     the mask byte are drawn. This requires the fact that two
     CGA pixels represent 1 VGA pixel in these routines.
     Assembly is definitely required here, in CGA mode, due to
     the bit manipulations. Format: [mask][data][data][mask][data][data]...
     */
int nr,nc,i1,i2,i3,i4,i5;
unsigned char x1,x2,x3,x4;

nr=(char)data[0];
nc=(char)data[1];
switch(scr_mode)
{ case 6 : /* CGA mono */
           x=x >> 2;  /* *2/8, or /4 for byte offset */
           i3=2; i4=7;
           if (y+nr>scrn_lines) nr=scrn_lines-y+1;  /* clip bottom */
           if (y<0) { i3=2+nc*(0-y); nr=nr+y; y=0; }         /* clip top */
           for (i1=0; i1<nr; i1++)
             for (i2=0; i2<nc; i2++)
             { if (i4==7) x1=data[i3++];
               x2=data[i3++]; 
               for (i5=0; i5<8; i5=i5+2)
               { x3=1<<i4--;
                 if (i4==-1) i4=7;
                 if (x1&x3)
                 { x3=_peek(line_table[y+i1]+x+i2,scrn_base);
                   x4=~(0xc0 >> i5);
                   x3=x3&x4;
                   x4=x2&(0xc0 >> i5);
                   x3=x3|x4;
                   _poke(x3,line_table[y+i1]+x+i2,scrn_base);
                 }
               }
             }
           break;
  case 19: /* VGA 256c */
           i3=2;
           if (y+nr>scrn_lines) nr=scrn_lines-y+1;  /* clip bottom */
           if (y<0) { i3=2+nc*(0-y); nr=nr+y; y=0; }         /* clip top */
           for (i1=0; i1<nr; i1++)
             for (i2=0; i2<nc; i2++)
               if (data[i3])
                 _poke(data[i3++],line_table[y+i1]+x+i2,scrn_base);
               else i3++;
}
}

makemask(data) char data[];
{ /* works only under CGA, converts the mono data in 'data' to the
     mask format used with CGA under 'putm'. Treats any 00 bit combination
     as background, so it's not very useful to grab images from the
     screen if you want them to be masked
     */
int i1,i3,i5,s,nr,nc;
char x1,x2,x3,ws[16000];

if (scr_mode!=6) return(0);  /* do nothing if not CGA */
nr=(char)data[0];
nc=(char)data[1];
i1=2; i5=0; s=nr*nc;
for (i3=0; i3<s; i3=i3+2)
{ x1=data[i1++];
  x2=data[i1++];
  x3=tst(x1&0xc0)*128+tst(x1&0x30)*64+tst(x1&0x0c)*32+tst(x1&0x03)*16
    +tst(x2&0xc0)*8+tst(x2&0x30)*4+tst(x2&0x0c)*2+tst(x2&0x03);
  ws[i5++]=x3;
  ws[i5++]=x1;
  ws[i5++]=x2;
}
i1=2;
for (i3=0; i3<i5; i3++)
  data[i1++]=ws[i3];
}

tst(x) int x;
{ /* test a number. Return 1 if non-zero, or 0 if equal to 0. */
if (x!=0) return(1);
     else return(0);
}

cls()
{ /* clear the screen, whatever mode */
unsigned i;

switch (scr_mode)
{ case 6 :for (i=0; i<16000; i++)
            _poke(0,i,scrn_base);
          break;
  case 19:for (i=0; i<64000; i++) 
            _poke(0,i,scrn_base);
          break;
  default:scr_clr();
}}

make_cga(dat) char dat[];
{ /* convert the VGA data in dat[] to CGA mono format.
     Palette for VGA should already be set up in PAL[].
     Converts each colour to mono pixels, either 00, 01, or 11 */
int nr,nc,i,i2,i3,i4,i5,j,o;

nr=dat[0]; nc=dat[1];
dat[1]=(char)(nc>>2); /* fix columns for CGA */
if (dat[1]<<2 < nc) (dat[1])++;

i=2; j=2;
for (i2=0; i2<nr; i2++)
  for (i3=0; i3<nc; i3=i3+4)
  { o=0;
    for (i4=6; i4>=0; i4=i4-2)
    { i5=i3+(3-(i4/2)); 
      if (i5<nc)
         o=o|((bits(dat[j++],i2))<<i4);
    }
    dat[i++]=o;
  }
}

bits(x,l) int x,l;
{ /* return the two mono bits (LSB) corresponding to the VGA pixel in x 
     and the line number 'l' */
int q,r,g,b,o;

r=pal[x*3]; g=pal[x*3+1]; b=pal[x*3+2];
q=((r*30)/100)+((g*59)/100)+((b*11)/100);

if (NO_BLACK)
  if (x!=0) q=q+12;

o=3;
if (q<49) {o=3; if (l>>1<<1!=l) o=2; }
if (q<36) o=2;
if (q<25) {o=1; if (l>>1<<1!=l) o=0; }
if (q<12) o=0;
return(o);
}
