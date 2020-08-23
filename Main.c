#include <REG51.H>
#include <stdio.h>  

int flag = 0;
int arr[5];
int index=0;
int freq[5];
int ampl[5];
int lookup[10] = {31, 15, 7, 3, 1, 0, 16, 24, 28, 30};
int i=0;
int check=1;


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

void frequency(){
	printf("freq");
	for(i=0; i<sizeof(arr); i++){
		freq[i] = arr[i];
	}
	flag=0;
}

void amplitude(){
	printf("ampl");
		for(i=0; i<sizeof(arr); i++){
		ampl[i] = arr[i];
	}
	flag=0;
}


void main (void)  {
	
	SCON = 0x52;    /* SCON */                   /* setup serial port control */
	TMOD = 0x20;    /* TMOD */                   /* hardware (2400 BAUD @12MHZ) */
	TCON = 0x69;    /* TCON */
	TH1 =  0xf3;    /* TH1 */
	
	TR1 = 1;      // Turn ON the timer for Baud rate generation
  ES  = 1;      // Enable Serial INterrupt
  EA  = 1;      // Enable Global Interrupt bit

	while(1){
		if (flag==1){			
			frequency();
		}else if(flag==2){
			amplitude();
		}
	}
}
