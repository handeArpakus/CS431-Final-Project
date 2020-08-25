#include <REG51.H>
#include <stdio.h>  

int flag = 0;
int serialFlag=0;
int arr[3];
int index=0;
int frequency=-1;
int amplitude=-1;
int second;
int i=0;
int innerFlag=0;

char buf [10];

void returnHome(void);
void entryModeSet(bit id, bit s);
void displayOnOffControl(bit display, bit cursor, bit blinking);
void cursorOrDisplayShift(bit sc, bit rl);
void functionSet(void);
void setDdRamAddress(char address);

void sendChar(char c);
void sendString(char* str);
bit getBit(char c, char bitNumber);
void delay(void);

bit getBit(char c, char bitNumber) {
	return (c >> bitNumber) & 1;
}

void delay(void) {
	char c;
	for (c = 0; c < 50; c++);
}

void sendChar(char c) {
	DB7 = getBit(c, 7);
	DB6 = getBit(c, 6);
	DB5 = getBit(c, 5);
	DB4 = getBit(c, 4);
	RS = 1;
	E = 1;
	E = 0;
	DB7 = getBit(c, 3);
	DB6 = getBit(c, 2);
	DB5 = getBit(c, 1);
	DB4 = getBit(c, 0);
	E = 1;
	E = 0;
	delay();
}

void sendString(char* str) {
	int index = 0;
	while (str[index] != 0) {
		sendChar(str[index]);
		index++;
	}
}

void functionSet(void) {
	// The high nibble for the function set is actually sent twice. Why? See 4-bit operation
	// on pages 39 and 42 of HD44780.pdf.
	DB7 = 0;
	DB6 = 0;
	DB5 = 1;
	DB4 = 0;
	RS = 0;
	E = 1;
	E = 0;
	delay();
	E = 1;
	E = 0;
	DB7 = 1;
	E = 1;
	E = 0;
	delay();
}

void setDdRamAddress(char address) {
	RS = 0;
	DB7 = 1;
	DB6 = getBit(address, 6);
	DB5 = getBit(address, 5);
	DB4 = getBit(address, 4);
	E = 1;
	E = 0;
	DB7 = getBit(address, 3);
	DB6 = getBit(address, 2);
	DB5 = getBit(address, 1);
	DB4 = getBit(address, 0);
	E = 1;
	E = 0;
	delay();
}

void entryModeSet(bit id, bit s) {
	RS = 0;
	DB7 = 0;
	DB6 = 0;
	DB5 = 0;
	DB4 = 0;
	E = 1;
	E = 0;
	DB6 = 1;
	DB5 = id;
	DB4 = s;
	E = 1;
	E = 0;
	delay();
}

void displayOnOffControl(bit display, bit cursor, bit blinking) {
	DB7 = 0;
	DB6 = 0;
	DB5 = 0;
	DB4 = 0;
	E = 1;
	E = 0;
	DB7 = 1;
	DB6 = display;
	DB5 = cursor;
	DB4 = blinking;
	E = 1;
	E = 0;
	delay();
}

static unsigned long overflow_count = 0;

void timer0_ISR (void) interrupt 1
{
	overflow_count++;   /* Increment the overflow count */
	flag=3; //to sen wave
}

void serial_isr() interrupt 4{	
	static char ch = '\0';	
	if(RI == 1)
	{
		flag=0;	
		serialFlag=0;		
		ch = SBUF;
		if(ch != '\*' && ch != '\#'){
			if(ch >= '0' && ch <= '9'){
				arr[index]=ch-'0';
				index++;
			}
						
		}else if(ch == '\*') {
			SBUF=arr;
			flag=1; //frequency number came
			serialFlag=1;
			index = 0; 
			i=0; 
		}else if (ch == '\#'){
			SBUF=arr;
			flag=2; //amplitude number came
			serialFlag=1;
			index = 0; 
			i=0;
		}
		
		RI=0;
		TI=1;
	
	}else if(TI==1) {
		TI=0;
		if(ch!='\0'){
			SBUF = ch;
			ch = '\0';
		}
	}
}


void take_frequency(){
	
	int freq =0;
	for(i=0; i<3; i++){
			freq = freq*10 + arr[i];
	}
	frequency = freq;
	
	//LCD PART
	sprintf(buf, "%d", freq);
	functionSet();
	entryModeSet(1, 0); // increment and no shift
	displayOnOffControl(1, 1, 1); // display on, cursor on and blinking on
	setDdRamAddress(0x00); // set address to start of second line
	sendString(buf);

	flag=0;
	serialFlag=0;
}

void take_amplitude(){
	int ampl=0;
	for(i=0; i<3; i++){
		ampl= ampl*10 + arr[i];
	}
	amplitude = ampl;
	second = ampl;

	//LCD PART
	sprintf(buf, "%d", amplitude);
	functionSet();
	entryModeSet(1, 0); // increment and no shift
	displayOnOffControl(1, 1, 1); // display on, cursor on and blinking on
	
	setDdRamAddress(0x40); // set address to start of second line
	sendString(buf);

	flag=0;
	serialFlag=0;
}


void sendWave(){
	if(serialFlag == 0){
		if(amplitude != -1 && frequency != -1)
		{
			if(second >= 0 && innerFlag==0)
			{
				WR = 0;
				P1 = second;
				second--;
			}
			else if (second<=amplitude){
				if(second==-1){
					second = 0;
				}
				innerFlag=1;
				WR = 0;
				P1 = second;
				second++;
			}
			if(second>amplitude){
				second=amplitude;
				innerFlag=0;
			}	
		}
		flag=0;
	}else{
		amplitude=-1;
		frequency=-1;
	}
}


void main (void)  {
	
	SCON = 0x52;    /* SCON */                   /* setup serial port control */
	TMOD = 0x20;    /* TMOD */                   /* hardware (2400 BAUD @12MHZ) */
	TCON = 0x69;    /* TCON */
	TH1 =  0xf3;    /* TH1 */
	
	TR1 = 1;      // Turn ON the timer for Baud rate generation
  ES  = 1;      // Enable Serial INterrupt
  EA  = 1;      // Enable Global Interrupt bit
	
	/*--------------------------------------
	Set Timer1 for 8-bit timer with reload
	(mode 2). The timer counts to 255,
	overflows, is reloaded with 156, and
	generates an interrupt.

	Set the Timer1 Run control bit.
	--------------------------------------*/
	WR = 1;
	P1=0;
	TMOD = (TMOD & 0x0F) | 0x20;  /* Set Mode (8-bit timer with reload) */
	TH0 = 256 - 100;              /* Reload TL1 to count 100 clocks */
	TL0 = TH0;
	ET0 = 1;                      /* Enable Timer 1 Interrupts */
	TR0 = 1;                      /* Start Timer 1 Running */
	EA = 1;                       /* Global Interrupt Enable */
	

	while(1){
		if (flag==1){			
			take_frequency();
		}else if(flag==2){
			take_amplitude();
		}else if(flag==3)
			sendWave();
	}
}
