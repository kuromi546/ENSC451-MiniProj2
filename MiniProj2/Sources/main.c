#include <mc9s12dp512.h>
#include <hidef.h>      /* common defines and macros */
#include <math.h>

//LCD Settings
#define LCD_DATA PORTK
#define LCD_CTRL PORTK
#define RS 0x01
#define EN 0x02

//Variables
const unsigned char keypad[4][4] =
{
'1','2','3','A',
'4','5','6','B',
'7','8','9','C',
'*','0','#','D'
};
const unsigned int f_base= 2;
unsigned char column,row;
unsigned char COUNT;
unsigned int f;

//Functions
void init(void);
void init_LCD(void);
void init_TSCR(unsigned int prescaler);
void init_SCI(void);
void COMWRT4(unsigned char);
void DATWRT4(unsigned char); 
void clearLCD(void);
void mSDelay(unsigned int);
void keypadinput(void);  
void SerTX(unsigned char); 
void send(unsigned int y,unsigned char x);
     
/**********************MAIN***********/
void main(void) {  
  DisableInterrupts;
  
  init();
  init_LCD();
  init_TSCR(128);   //1,2,4,8,16,32,64,128 
  init_SCI();
  
  EnableInterrupts;
  
  for(;;);     
}

/**********************INTERRUPTS***********/

interrupt (((0x10000-Vtimch7)/2)-1) void TC7_ISR(void)  {    
  //Output Compare 7, interrupt for LEDs
  TC7 = TCNT + (word)62500/f; 
  PORTB++;  
  TFLG1 = TFLG1 | TFLG1_C7F_MASK; 
}

interrupt (((0x10000-Vtimovf)/2)-1) void TOF_ISR(void) {    
  //Timer Over Flow, interrupt for keypad
  keypadinput();
  TFLG2 = TFLG2 | TFLG2_TOF_MASK; 
}

/**********************SERIAL PORT***********/

void init_SCI(void)  {
  SCI0BDH=0x00;   //Serial Monitor used for LOAD works at 48MHz  
  SCI0BDL=0x68;   //8MHz/2=4MHz, 4MHz/128=31,250 and 31,250/2400=13
  SCI0CR1=0x00;
  SCI0CR2=0x0C;
}

void SerTX(unsigned char x) { //Send Character to Serial Output 
  while(!(SCI0SR1 & SCI0SR1_TDRE_MASK)); 
  SCI0DRL=x;				
} 
                                                       
/**********************SUBROUTINES***********/

void mSDelay(unsigned int itime)  { //Delay
  unsigned int i; unsigned int j;
  for(i=0;i<itime;i++)
    for(j=0;j<4000;j++);
}

void init() {     //Initialize Ports/Values
  f = f_base; 
  COUNT = 0;
  
  DDRA = 0x0F;    //PORTA = output for keypad
  DDRB = 0xFF;    //PORTB = output for LEDs
  DDRH = 0x00;    //PORTH = output for switches
  DDRJ |=0x02;    //PTJ = output for LEDs
  DDRK = 0xFF;    //PORTK = output for LCD 
  DDRP |=0x0F;    //PTP = output for 7-segment display, PTP = 0xFF to turn off
  DDRT = 0xFF;    //PTT = output for Buzzer
  
  PTJ &=~0x02;
  PTP |=0x0F;
}

void init_TSCR(unsigned int prescaler)	{ //Setup Timers, enable interrupts
  TSCR1 = 0x80;
  switch (prescaler)  {     
    case 1:
      TSCR2 = 0x80;
      break;
    case 2:
      TSCR2 = 0x81;
      break;
    case 4:
      TSCR2 = 0x82;
      break;
    case 8:
      TSCR2 = 0x83;
      break;
    case 16:
      TSCR2 = 0x84;
      break;
    case 32:
      TSCR2 = 0x85;
      break;
    case 64:
      TSCR2 = 0x86;
      break;
    case 128:
      TSCR2 = 0x87;
      break;
  }
  TFLG2 = TFLG2 | TFLG2_TOF_MASK;
  TIOS = TIOS | TIOS_IOS7_MASK; 
  TCTL1 = 0x40; 
  TIE = 0x80;
}
  
/**********************LCD***********/

void init_LCD()    //Initialize LCD
{
  COMWRT4(0x33);   //reset sequence provided by data sheet
  mSDelay(1);
  COMWRT4(0x32);   //reset sequence provided by data sheet
  mSDelay(1);
  COMWRT4(0x28);   //Function set to four bit data length
                   //2 line, 5 x 7 dot format
  clearLCD();
}

void COMWRT4(unsigned char command) { //For LCD
  unsigned char x;
        
  x = (command & 0xF0) >> 2;         //shift high nibble to center of byte for Pk5-Pk2
  LCD_DATA =LCD_DATA & ~0x3C;          //clear bits Pk5-Pk2
  LCD_DATA = LCD_DATA | x;           //sends high nibble to PORTK
  mSDelay(1);
  LCD_CTRL = LCD_CTRL & ~RS;         //set RS to command (RS=0)
  mSDelay(1);
  LCD_CTRL = LCD_CTRL | EN;          //rais enable
  mSDelay(5);
  LCD_CTRL = LCD_CTRL & ~EN;         //Drop enable to capture command
  mSDelay(15);                       //wait
  x = (command & 0x0F) << 2;          // shift low nibble to center of byte for Pk5-Pk2
  LCD_DATA =LCD_DATA & ~0x3C;         //clear bits Pk5-Pk2
  LCD_DATA =LCD_DATA | x;             //send low nibble to PORTK
  LCD_CTRL = LCD_CTRL | EN;          //rais enable
  mSDelay(5);
  LCD_CTRL = LCD_CTRL & ~EN;         //drop enable to capture command
  mSDelay(15);
}

void DATWRT4(unsigned char data) { //For LCD
  unsigned char x;
     
  x = (data & 0xF0) >> 2;
  LCD_DATA =LCD_DATA & ~0x3C;                     
  LCD_DATA = LCD_DATA | x;
  mSDelay(1);
  LCD_CTRL = LCD_CTRL | RS;
  mSDelay(1);
  LCD_CTRL = LCD_CTRL | EN;
  mSDelay(1);
  LCD_CTRL = LCD_CTRL & ~EN;
  mSDelay(5);
       
  x = (data & 0x0F)<< 2;
  LCD_DATA =LCD_DATA & ~0x3C;                     
  LCD_DATA = LCD_DATA | x;
  LCD_CTRL = LCD_CTRL | EN;
  mSDelay(1);
  LCD_CTRL = LCD_CTRL & ~EN;
  mSDelay(15);
}

void clearLCD(void) { //For LCD
  mSDelay(1);
  COMWRT4(0x06);  //entry mode set, increment, no shift
  mSDelay(1);
  COMWRT4(0x0E);  //Display set, disp on, cursor on, blink off
  mSDelay(1);
  COMWRT4(0x01);  //Clear display
  mSDelay(1);
  COMWRT4(0x80);  //set start posistion, home position
  mSDelay(1);
  PORTB = 0x00;
}

/**********************KEYPAD***********/

void keypadinput(void) {
  if(row == 0x00)  {
    PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
    row = PORTA & 0xF0;              //READ ROWS
  }
  
  while(1){                           //OPEN while(1)
     PORTA &= 0xF0;                   //CLEAR COLUMN
     PORTA |= 0x01;                   //COLUMN 0 SET HIGH
     row = PORTA & 0xF0;              //READ ROWS
     if(row != 0x00){                 //KEY IS IN COLUMN 0
        column = 0;
        break;                        //BREAK OUT OF while(1)
     }
     PORTA &= 0xF0;                   //CLEAR COLUMN
     PORTA |= 0x02;                   //COLUMN 1 SET HIGH
     row = PORTA & 0xF0;              //READ ROWS
     if(row != 0x00){                 //KEY IS IN COLUMN 1
        column = 1;
        break;                        //BREAK OUT OF while(1)
     }

     PORTA &= 0xF0;                   //CLEAR COLUMN
     PORTA |= 0x04;                   //COLUMN 2 SET HIGH
     row = PORTA & 0xF0;              //READ ROWS
     if(row != 0x00){                 //KEY IS IN COLUMN 2
        column = 2;
        break;                        //BREAK OUT OF while(1)
     }
     PORTA &= 0xF0;                   //CLEAR COLUMN
     PORTA |= 0x08;                   //COLUMN 3 SET HIGH
     row = PORTA & 0xF0;              //READ ROWS
     if(row != 0x00){                 //KEY IS IN COLUMN 3
        column = 3;
        break;                        //BREAK OUT OF while(1)
     }
     row = 0;                         //KEY NOT FOUND
    break;                              //step out of while(1) loop to not get stuck
  }                                   //end while(1)

  if(row == 0x10){
    clearLCD();
    if (column == 0x00)  {
       f=power(f_base,1);
       send(0,column);
    } else if(column == 0x01)  {
       f=power(f_base,2);
       send(0,column);
    } else if(column == 0x02)  {
       f=power(f_base,3);
       send(0,column);
    } else if(column == 0x03)  {
       PORTB = 0xFF;
       f=f_base;
       DATWRT4('H');
       DATWRT4('z');
    }           
  }
  else if(row == 0x20){
     clearLCD();
     if (column == 0x00)  {
        f=power(f_base,4);
        send(1,column);
     } else if(column == 0x01)  {
        f=power(f_base,5);
        send(1,column);
     } else if(column == 0x02)  {
        f=power(f_base,6);
        send(1,column);
     } else if(column == 0x03)  {
        PORTB = 0xFF;
        f=f_base;
        DATWRT4('k');
        DATWRT4('H');
        DATWRT4('z');
     }         
  }
  else if(row == 0x40){
    clearLCD();
    if (column == 0x00)  {
       f=power(f_base,7);
       send(2,column);
    } else if(column == 0x01)  {
       f=power(f_base,8);
       send(2,column);
    } else if(column == 0x02)  {
       f=power(f_base,9);
       send(2,column);
    } else if(column == 0x03)  {
       PORTB = 0xFF;
       f=f_base;
       DATWRT4('C');
       DATWRT4('L');
       DATWRT4('E');
       DATWRT4('A');
       DATWRT4('R');
       clearLCD();
    } 
  }
  else if(row == 0x80){
     clearLCD();
     if (column == 0x00)  {
        PORTB = 0xFF;
        f=f_base;
        send(3,column);
     } else if(column == 0x01)  {
        PORTB = 0xFF;
        f=f_base;
        send(3,column);
     } else if(column == 0x02)  {
        PORTB = 0xFF;
        f=f_base;
        send(3,column);
     } else if(column == 0x03)  {
        PORTB = 0xFF;
        f=f_base;
        DATWRT4('S');
        DATWRT4('E');
        DATWRT4('T');
     } 
  } 
  
  if(row != 0x00)  {
    mSDelay(1);
    PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
    row = PORTA & 0xF0;              //READ ROWS
  }
}

void send(unsigned int y,unsigned char x)  {
  DATWRT4(keypad[y][x]);
  //PORTB=keypad[y][x];         //OUTPUT TO PORTB LED
  SerTX(keypad[y][x]);        //OUTPUT TO SCI0
  SerTX('\n');
}

int power(unsigned int x, unsigned int y)  {
  int i;
  int z = x;
  for(i=0;i<y;i++)  {
    z = z*2;    
  }
  return z;
}