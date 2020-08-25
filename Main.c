#include <REG51.H>
#include <stdio.h>  

int flag = 0;
int arr[3];
int index=0;
int frequency=-1;
int amplitude=-1;
int lookup[10] = {31, 15, 7, 3, 1, 0, 16, 24, 28, 30};
int i=0;
int check=1;

sbit clear = P2^4;
sbit ret = P2^5;				  
sbit left = P2^6;
sbit right = P2^7;
sbit A1 = P3^3;
sbit A0 = P3^4;
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
	flag=3; //for update the amplitude
}

void serial_isr() interrupt 4{	
	static char ch = '\0';	
	if(RI == 1)
	{
		flag=0;		
		ch = SBUF;
		if(ch != '\*' && ch != '\#'){
			if(ch >= '0' && ch <= '9'){
				arr[index]=ch-'0';
				index++;
			}
						
		}else if(ch == '\*') {
			SBUF=arr;
			flag=1; //frequency number came
			index = 0; 
			i=0; 
		}else if (ch == '\#'){
			SBUF=arr;
			flag=2; //amplitude number came
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


int i;



void take_frequency(){
	int freq =0;

	printf("\n i: %d", sizeof(arr));
	
	for(i=0; i<3; i++){
			freq = freq*10 + arr[i];
		printf("\ni %d: %d\n", i, arr[i]);  
	}
	
	//sendString(freq);
	printf("\nfreq: %d\n", freq); 
	flag=0;
}

void take_amplitude(){
	int ampl=0;
 	for(i=0; i<3; i++){
		ampl= ampl*10 + arr[i];
}
	printf("\ampl: %d\n", ampl);
	amplitude = ampl;
	flag=0;
}

void sendAmpl(){
	int second = amplitude;
	if(amplitude != -1)
	{
			while(second >= 0)
			{
				printf("\n amp: %d\n",second );
				WR = 0;
				P1 = second;
				second--;
			}
			while(second<=amplitude){
				printf("\n amp: %d\n",second );
				WR = 0;
				P1 = second;
				second++;
			}
		
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
			sendAmpl();
	}
}
