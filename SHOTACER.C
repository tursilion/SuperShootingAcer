/* Super Shooting Acer - A little diversion... */

char HELL[10];

int CGA,x,y,z,k,step,score,de;
int QUIT,miss,high;

int p[5],t[5],fl[5];

char shapes[40590];

/* Mike's subroutines for use with the Personal C Compiler */
/* added to here and there, as needed */

#include <stdio.h>

extern int scr_mode;  /* extern to PCIOs mode variable */
extern unsigned _rax,_rbx; /* interrupt access registers */

int speed; /* holds value used by delay for 1/60 second */

int line_table[200]; /* holds offsets of 200 screen lines */
int scrn_base;   /* holds segment base of screen */
int scrn_lines;  /* holds max line # on the screen (normally 199) */

int NO_BLACK; /* if set, diables black for CGA conversion */

char pal[768],tp[768];  /* holds a VGA palette. Wasted space under CGA */

mike_init()
{ /* initialize whatever needs to be initialized here */
set_speed();
scr_setup();  /* must link with PCIO! */
NO_BLACK=0;   /* allow black */
}

set_speed()
{ /* set the variable "speed" based on the machine */
char x1[10],x2[10];
int q, q2;

q=0; q2=0;             /* counter */
times(x1);             /* get current time */
times(x2);             /* get it again (HHMMSS) */
while (x2[7]==x1[7])   /* compare seconds */
 times(x1);            /* get new time */
times(x2);             /* now we're starting at the beginning of a second */
while (x2[7]==x1[7])   /* wait for the seconds to change again */
{
q++;
if (q==0) q2++;
times(x2);
}                       /* while counting and getting new time */
speed=(q/60)+(q2*1024); /* q loops in 1 second. Divide by 60 for jiffies */
}

delay(x) int x;
{ /* pauses */
int a;  char x2[10];

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


bits(x,l) int x,l;
{ /* return the two mono bits (LSB) corresponding to the VGA pixel in x 
     and the line number 'l' */
int q,r,g,b,o;

r=pal[x*3]; g=pal[x*3+1]; b=pal[x*3+2];
q=((r*30)/100)+((g*59)/100)+((b*11)/100);

if (NO_BLACK)
  if (x!=0) q=q+12;

/* color 0 is allowed to be black */

o=3;
if (q<49) {o=3; if (l>>1<<1!=l) o=2; }
if (q<36) o=2;
if (q<25) {o=1; if (l>>1<<1!=l) o=0; }
if (q<12) o=0;
return(o);
}

vprint(x,y,c,s) int x,y,c; char *s;
{ /* print a VGA string at x,y, color c */
  /* c|0x100 = solid background */
unsigned int q,i,j,f; unsigned char w,m;

if (CGA) 
{ x=x/4; 
  y=y/8;
  scr_rowcol(y,x);
  puts(s);
  return;
}

f=0;
if (c&0x100)
{ f=1;
  c=c&0xff;
}

while (*s)
{ q=(*s++)*8+0xe;
  for (i=0; i<8; i++)
  { w=_peek(q++,0xffa6);
    m=0x80;
    for (j=0; j<8; j++)
    { if (w&m) _poke(c,line_table[y+i]+x+j,scrn_base);
          else if (f) _poke(0,line_table[y+i]+x+j,scrn_base); 
      m=m>>1;
    }
  }
  x=x+8;
} /* while */
}

load_bmp(n) char *n;
{ /* load a 320x200x8 BMP file onto the screen. Load palette, but don't
     set it */
FILE *fp;
int x,y;
int r,g,b;

fp=fopen(n,"rb");

if (fp==NULL) fail("cannot open");

for (x=0; x<256; x++)
  colour(x,0,0,0);   /* black palette */

for (x=0; x<54; x++) 
  fgetc(fp);         /* skip over the header */

y=0;
for (x=0; x<256; x++)
{ /* read palette */
  b=fgetc(fp)>>2;
  g=fgetc(fp)>>2;  /* shifted for VGA constraints */
  r=fgetc(fp)>>2;
  fgetc(fp);
  tp[y++]=(char)r;
  tp[y++]=(char)g;
  tp[y++]=(char)b;
}

if (CGA)
{ for (y=0; y<768; y++)
    pal[y]=tp[y];
  pal[0]=0;
  pal[1]=0;
  pal[2]=0;  /* black background */
  /* need palette to dither pic */
  
  for (y=199; y>=0; y--)
    for (x=0; x<80; x++)
    { r=(char)((bits(fgetc(fp),y)<<6)|(bits(fgetc(fp),y)<<4)|
               (bits(fgetc(fp),y)<<2)|(bits(fgetc(fp),y)));
      _poke(r,line_table[y]+x,scrn_base);
    }
} else
{ for (y=199; y>=0; y--)
  { /* read lines */
    for (x=0; x<320; x++)
      /* read one line into screen buffer */
      _poke(fgetc(fp),line_table[y]+x,scrn_base);
  }
}

/* all done */
fclose(fp);
}

fade_in(x) int x;
{ /* fade palette in tp[] to pal[], speed x */
int y,z,r,g,b;

if (CGA) return;

for (y=63; y>=0; y--)
{ for (z=0; z<256; z++)
  { r=tp[z*3]-y;
    g=tp[z*3+1]-y;
    b=tp[z*3+2]-y;
    if (r<0) r=0;
    if (g<0) g=0;
    if (b<0) b=0;
    colour(z,r,g,b);
  }
  delay(speed/x);
}
}

fade_out(x) int x;
{ /* fade palette out... tp[] must be equal to pal[], speed x */
int y,z,r,g,b;

if (CGA) return;

for (y=1; y<64; y++)
{ for (z=0; z<256; z++)
  { r=tp[z*3]-y;
    g=tp[z*3+1]-y;
    b=tp[z*3+2]-y;
    if (r<0) r=0;
    if (g<0) g=0;
    if (b<0) b=0;
    colour(z,r,g,b);
  }
  delay(speed/x);
}
}

/******************** game starts here ************************/

fail(x) char *x;
{ puts(x);
  exit(5);
}

main(argc,argv) int argc; char *argv[];
{ /* not a very serious game, just a bit of fun.. */
char save[768];    /* save palette */

printf("\nInitializing...\n");

if (*argv[1]=='c')
{ printf("CGA Mode!\n");
  CGA=1;
}

if (strcmp(argv[1],"-greet")==0)
  greet();

mike_init();

strcpy(HELL,"HOT!!");

score=0;
high=0;

while (HELL!="Frozen")
{
set_txt();
cls();
sound_off();
while (scr_csts());

center(1,"* Super Shooting Acer *",3);
center(2,"(a mindless diversion (c)1995 by Mike Brent)",5);
center(4,"What to do: Shoot the bad guys",1);
center(6,"How to do it: On the next screen you will",6);
center(7,"see the characters involved. Press a key when",6);
center(8,"ready. You will now see 5 boxes numbered 1-5.",6);
center(9,"Characters will appear in these boxes. To",6);
center(10,"shoot, press the appropriate number key.",6);
center(11,"Remember to shoot ONLY bad guys!",2);
center(13,"This game is ???Ware. If you like it at all, send me something!",7);
center(14,"Money, bridges, fame, dolphins, thank you cards, coca-cola! :)",7);
center(16,"M. Brent",4);
center(17,"113-437 Martin St, Suite 245",4);
center(18,"Penticton, BC, Canada",4);
center(19,"V2A 5L1",4);
center(21,"EMail: mbrent@awinc.com  WWW: http://www2.awinc.com/users/mbrent",5);
center(23,"Press any key to view characters",16);

if (score>high) high=score;

while((k=scr_csts())==0)
  rand();

if (k==27) shutdown();

if (CGA)
  set_cga();
else
  set_vga();

load_bmp("all.bmp");
fade_in(2);

get(&shapes[0],0,0,75,60);
get(&shapes[4510],80,0,75,60);
get(&shapes[9020],160,0,75,60);
get(&shapes[13530],240,0,75,60);
get(&shapes[18040],0,100,75,60);
get(&shapes[22550],80,100,75,60);
get(&shapes[27060],160,100,75,60);
get(&shapes[31570],240,100,75,60);

for (x=0; x<4510; x++)
  shapes[36080+x]=255;       /* blank is made of colour 255 */
shapes[36080]=shapes[0];
shapes[36081]=shapes[1];

vprint(72,191,2,"Press any key to begin");

while ((k=scr_csts())==0);

if (k==27) shutdown();

fade_out(2);

for (x=0; x<768; x++)
  save[x]=tp[x];       /* save palette */

load_bmp("screen.bmp");

for (x=0; x<768; x++)
  tp[x]=save[x];       /* fool my own routines and restore old palette */
tp[765]=63;
tp[766]=63;
tp[767]=63;  /* ensure last colour is set correctly */

fade_in(2);

game();

}/* ever */
}

center(r,s,c) int r; char *s; int c;
{ /* print a centered string (text mode) at row r in colour c */
scr_rowcol(r,(80-strlen(s))/2);
scr_aputs(s,c);
}

shutdown()
{ /* exit game */
set_txt();
sound_off();
printf("\nLast score: %d\n\n",score);
printf("Session high score: %d\n",high);

puts("\nThanks for playing!!\n\n");

exit(0);
}

game()
{ /* play the game! */
int r;

for (x=0; x<5; x++)
{ p[x]=0; t[x]=-1; }
step=1;
de=speed/5;
if (de==0)
{ scr_rowcol(4,7);
  puts("* WARNING - Slow Machine *");
  de=1;
}
score=0;
miss=0;
vprint(24,166,0,"Miss:");
vprint(24,174,0,"Score:");
vprint(24,186,0,"High :");
nprint(88,186,255|0x100,high);
QUIT=1;
while (QUIT)
{ for (z=0; z<5; z++)
  { if (t[z]!=-1)
    { x=t[z]*4510;
      p[z]=p[z]+step;
      if (p[z]>75) p[z]=75;
      shapes[x]=p[z];
      put(&shapes[x],z*63+2,137-p[z]);
      sound_on(((75-p[z])*100)+1000);
      if (p[z]==75)
      { if (t[z]>3) score=score+10;
        else missbad(z);
        put(&shapes[36080],z*63+2,62);
        t[z]=-1;
        sound_off();
      }
    }
    delay(de);
    k=scr_csts();
    if (k==27) QUIT=0;   /* escape with ESC */
    if ((k>='1')&&(k<='5'))
    { sound_on(450);
      z=k-49;
      if (t[z]!=-1)
      { x=t[z]*4510;
        shapes[x]=75;
        put(&shapes[x],z*63+2,62);
      }
      invert(z);
      if (t[z]>3) badhit(z);
      if ((t[z]<4)&&(t[z]!=-1)) score=score+20;
      t[z]=-1;
      fl[z]=1;
      sound_off();
    }
  }
  r++;
  nprint(88,176,255|0x100,score);

  step=(score/1000)+1;
  if (step>75) step=75;

  for (z=0; z<5; z++)
  { if (fl[z])
      put(&shapes[36080],z*63+2,62);
     fl[z]=0;
  }

  if (((rand()/(score+1))<10)||(r>=100))
  { z=rand()%5;
    if (t[z]==-1)
    { t[z]=rand()%8;
      p[z]=0;
      r=0;
    }
  }
}
}

nprint(x,y,c,z) int x,y,c,z;
{ /* print a number with vprint */
char s[80];
int i;

i=0;

if (z/10000)
{ s[i++]=(z/10000)+48;
  z=z%10000;
}

if ((z/1000)||(i))
{ s[i++]=(z/1000)+48;
  z=z%1000;
}

if ((z/100)||(i))
{ s[i++]=(z/100)+48;
  z=z%100;
}

if ((z/10)||(i))
{ s[i++]=(z/10)+48;
  z=z%10;
}

s[i++]=z+48;
s[i++]=0;

vprint(x,y,c,s);
}

missbad(z) int z;
{ /* failed to shoot a bad guy in time */
char buf[4510];
int i;

vprint(z*63+14,96,0,"MISS!");
vprint(z*63+13,95,255,"MISS!");
get(buf,z*63+2,62,75,60);
for (i=0; i<5; i++)
{ put(&shapes[36080],z*63+2,62);
  delay(speed*15);
  put(buf,z*63+2,62);
  play(220,15);
}
miss++;
vprint(56+16*miss,166,0,"X");
if (miss==3) gameover();
}

invert(z) int z;
{ /* invert the colours of a box.. probably look weird... s'ok :) */
char buf[4510];
int i;

get(buf,z*63+2,62,75,60);
for (i=2; i<4510; i++)
  buf[i]=~(buf[i]);
put(buf,z*63+2,62);
}

badhit(z) int z;
{ /* shot a good guy */
char buf[4510];
int i;

invert(z);
k=0;
vprint(z*63+22,96,0,"NO!");
vprint(z*63+21,95,255,"NO!");
get(buf,z*63+2,62,75,60);
for (i=0; i<5; i++)
{ put(&shapes[36080],z*63+2,62);
  delay(speed*15);
  put(buf,z*63+2,62);
  play(220,15);
}
miss++;
vprint(56+16*miss,166,0,"X");
if (miss==3) gameover();
}

gameover()
{ /* do the gameover routine */
char buf[830];
int x,y,xd,yd;

QUIT=0;  /* done game loop */

vprint(125,97,255,"GAME OVER");
vprint(123,97,255,"GAME OVER");
vprint(123,95,255,"GAME OVER");
vprint(125,95,255,"GAME OVER");
vprint(124,96,0,"GAME OVER");
sound_off();
get(buf,122,94,11,75);
while(scr_csts());
x=123; y=95;
xd=1; yd=1;
while ((k=scr_csts())==0)
{ x=x+xd;
  if ((x>244)||(x<1)) xd=-xd;
  y=y+yd;
  if ((y>188)||(y<1)) yd=-yd;
  put(buf,x,y);
  delay(speed);
  vprint(0,190,255|0x100,"('?' for credits, any other key to exit)");
}
if (k==27) shutdown();
if (k=='?') credits();
}

wait()
{ /* used in credits */

vprint(64,190,250,"Press any key to go on");
fade_in(3);
while (scr_csts()==0);
fade_out(3);
cls();
}

credits()
{ fade_out(3);
  cls();
  for (z=0; z<8; z++)
    shapes[z*4510]=75;
  put(&shapes[0],90,62);
  put(&shapes[18040],150,62);
  vprint(96,153,255,"Daniel Cormier");
  wait();
  put(&shapes[4510],90,62);
  put(&shapes[31570],150,62);
  vprint(112,153,255,"Mike Brent");
  wait();
  put(&shapes[9020],120,62);
  vprint(92,153,255,"Michelle Taylor");
  wait();
  put(&shapes[13530],120,62);
  vprint(108,153,255,"Steve Brent");
  wait();
  put(&shapes[22550],120,62);
  vprint(96,153,255,"Jay Swackhamer");
  wait();
  put(&shapes[27060],120,62);
  vprint(92,153,255,"Lawrence Wright");
  wait();
  vprint(24,92,255,"Concept, design & programming by");
  vprint(112,100,255,"Mike Brent");
  vprint(44,140,255,"(C)1995 All rights reserved");
  wait();
}

greet()
{ /* greeting function! */
printf("\n\nVery short greet function, as I've nothing really new to say,\n");
printf("I said it all in Super Sales Acer. It's now 15 Dec 1995.\n");
printf("This game was a lot more fun to make... it's fast and tough.\n");
printf("Hope you enjoy it!\n");
printf("Special 'hi' to.... ummm... well, everyone who I pictured in\n");
printf("this game... Steve! What *is* Michelle's last name????? :) \n");
printf("[oh! Never mind.... I called and found out :) \n");
exit(0);
}
