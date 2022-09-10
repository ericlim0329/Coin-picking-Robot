#include <XC.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
 
// Configuration Bits (somehow XC32 takes care of this)
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) 
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK
#pragma config FSOSCEN = OFF        // Turn off secondary oscillator on A4 and B4

// Defines
#define SYSCLK 40000000L
#define FREQ 100000L // We need the ISR for timer 1 every 10 us
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

volatile int ISR_pwm1=95, ISR_pwm2=240, ISR_cnt=0;

// The Interrupt Service Routine for timer 1 is used to generate one or more standard
// PWM control signals.  The servo signal has a fixed period of 20ms and a pulse width
// between 0.6ms and 2.4ms.



//FUNDEMENTAL FUNCTIONS


void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1_Handler(void)
{
	IFS0CLR=_IFS0_T1IF_MASK; // Clear timer 1 interrupt flag, bit 4 of IFS0

	ISR_cnt++;
	if(ISR_cnt==ISR_pwm1)
	{
		LATAbits.LATA3 = 0;
	}
	if(ISR_cnt==ISR_pwm2)
	{
		LATBbits.LATB6 = 0;
	}
	if(ISR_cnt>=2000)
	{
		ISR_cnt=0; // 2000 * 10us=20ms
		LATAbits.LATA3 = 1;
		LATBbits.LATB6 = 1;
	}
}

void SetupTimer1 (void)
{

	__builtin_disable_interrupts();
	PR1 =(SYSCLK/FREQ)-1; // since SYSCLK/FREQ = PS*(PR1+1)
	TMR1 = 0;
	T1CONbits.TCKPS = 0; // 3=1:256 prescale value, 2=1:64 prescale value, 1=1:8 prescale value, 0=1:1 prescale value
	T1CONbits.TCS = 0; // Clock source
	T1CONbits.ON = 1;
	IPC1bits.T1IP = 5;
	IPC1bits.T1IS = 0;
	IFS0bits.T1IF = 0;
	IEC0bits.T1IE = 1;
	
	INTCONbits.MVEC = 1; //Int multi-vector
	__builtin_enable_interrupts();
}

// Use the core timer to wait for 1 ms.
void wait_1ms(void)
{
    unsigned int ui;
    _CP0_SET_COUNT(0); // resets the core timer count

    // get the core timer count
    while ( _CP0_GET_COUNT() < (SYSCLK/(2*1000)) );
}

void waitms(int len)
{
	while(len--) wait_1ms();
}

#define PIN_PERIOD (PORTB&(1<<5))
#define joystick2_button (PORTB&(1<<14))

//determines the period of a square wave signal
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}

    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}
	
    _CP0_SET_COUNT(0); // resets the core timer count
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
	}

	return  _CP0_GET_COUNT();
}
 
void UART2Configure(int baud_rate)
{
    // Peripheral Pin Select
    U2RXRbits.U2RXR = 4;    //SET RX to RB8
    RPB9Rbits.RPB9R = 2;    //SET RB9 to TX

    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX
    U2BRG = Baud2BRG(baud_rate); // U2BRG = (FPb / (16*baud)) - 1
    
    U2MODESET = 0x8000;     // enable UART2
}

void uart_puts(char * s)
{
	while(*s)
	{
		putchar(*s);
		s++;
	}
}

char HexDigit[]="0123456789ABCDEF";
void PrintNumber(long int val, int Base, int digits)
{ 
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (val>0) | (digits>0) )
	{
		buff[j--]=HexDigit[val%Base];
		val/=Base;
		if(digits!=0) digits--;
	}
	uart_puts(&buff[j+1]);
}

//configure analog-digital converters
void ADCConf(void)
{
    AD1CON1CLR = 0x8000;    // disable ADC before configuration
    AD1CON1 = 0x00E0;       // internal counter ends sampling and starts conversion (auto-convert), manual sample
    AD1CON2 = 0;            // AD1CON2<15:13> set voltage reference to pins AVSS/AVDD
    AD1CON3 = 0x0f01;       // TAD = 4*TPB, acquisition time = 15*TAD 
    AD1CON1SET=0x8000;      // Enable ADC
}

int ADCRead(char analogPIN)
{
    AD1CHS = analogPIN << 16;    // AD1CHS<16:19> controls which analog pin goes to the ADC
 
    AD1CON1bits.SAMP = 1;        // Begin sampling
    while(AD1CON1bits.SAMP);     // wait until acquisition is done
    while(!AD1CON1bits.DONE);    // wait until conversion done
 
    return ADC1BUF0;             // result stored in ADC1BUF0
}

//configures I/O pins
void ConfigurePins(void)
{
    // Configure pins as analog inputs
    ANSELBbits.ANSB2 = 1;   // set RB2 (AN4, pin 6 of DIP28) as analog pin
    TRISBbits.TRISB2 = 1;   // set RB2 as an input
    ANSELBbits.ANSB3 = 1;   // set RB3 (AN5, pin 7 of DIP28) as analog pin
    TRISBbits.TRISB3 = 1;   // set RB3 as an input
    ANSELBbits.ANSB12 = 1;   
    TRISBbits.TRISB12 = 1;
    ANSELBbits.ANSB13 = 1;   
    TRISBbits.TRISB13 = 1;
    
    
    
	// Configure digital input pin to measure signal period
	ANSELB &= ~(1<<5); // Set RB5 as a digital I/O (pin 14 of DIP28)
    TRISB |= (1<<5);   // configure pin RB5 as input
    CNPUB |= (1<<5);   // Enable pull-up resistor for RB5
    
   // ANSELB &= ~(1<<14); // Set RB14 as a digital I/O (pin 25 of DIP28)
    TRISBbits.TRISB14 = 1;   // configure pin RB14 as input
  //  CNPUB |= (1<<14);   // Enable pull-up resistor for RB14
    
    // Configure output pins
	TRISAbits.TRISA0 = 0; // pin  2 of DIP28
	TRISAbits.TRISA1 = 0; // pin  3 of DIP28
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
	TRISAbits.TRISA2 = 0; // pin  9 of DIP28
	TRISAbits.TRISA3 = 0; // pin 10 of DIP28
	TRISBbits.TRISB4 = 0; // pin 11 of DIP28
	TRISBbits.TRISB6 = 0; //RB6 is output for arm raise servo
	TRISBbits.TRISB15 = 0; //RB6 is output for arm raise servo

	INTCONbits.MVEC = 1;
}


//H BRIDGE FUNCTIONS FOR CONTROLLING WHEEL MOTORS

void turn_left() {

	LATAbits.LATA0 = 0;		// set RA0 as 0
	LATAbits.LATA1 = 1;		// set RA1 as 1
	LATBbits.LATB0 = 0;		// set RB0 as 0
	LATBbits.LATB1 = 1;		// set RB1 as 1

}


void turn_right() {


	LATAbits.LATA0 = 1;		// set RA0 as 1
	LATAbits.LATA1 = 0;		// set RA1 as 0
	LATBbits.LATB0 = 1;		// set RB0 as 1
	LATBbits.LATB1 = 0;		// set RB1 as 0

}

void move_backwards() {

	LATAbits.LATA0 = 0;		// set RA0 as 0
	LATAbits.LATA1 = 1;		// set RA1 as 1
	LATBbits.LATB0 = 1;		// set RB0 as 1
	LATBbits.LATB1 = 0;		// set RB1 as 0

}

void move_forwards() {

	LATAbits.LATA0 = 1;		// set RA0 as 1
	LATAbits.LATA1 = 0;		// set RA1 as 0
	LATBbits.LATB0 = 0;		// set RB0 as 0
	LATBbits.LATB1 = 1;		// set RB1 as 1

}

void stop() {

	LATAbits.LATA0 = 0;		// set RA0 as 0
	LATAbits.LATA1 = 0;		// set RA1 as 0
	LATBbits.LATB0 = 0;		// set RB0 as 0
	LATBbits.LATB1 = 0;		// set RB1 as 0

}


//SERVO COIN PICKUP ROUTINE
//NOT USED IN THIS VERSION OF THE CODE

void pick_up_coin(){

int i;
		waitms(250);
		ISR_pwm1=95;
		waitms(250);

		ISR_pwm2=240;
		waitms(250);

		ISR_pwm1=60;
		waitms(1000);

		ISR_pwm2=90;
		//turn magnet on
		waitms(1000);

		LATBbits.LATB15 = 1;
	
	
		//turn ISR_pwm1 (base of arm) from 60 to 200 over a few seconds to sweep ground
		waitms(100);
		waitms(100);
		ISR_pwm1=70;
		waitms(100);
		ISR_pwm1=80;
		waitms(100);
		ISR_pwm1=90;
		waitms(100);
		ISR_pwm1=100;
		waitms(100);
		ISR_pwm1=110;
		waitms(100);
		ISR_pwm1=120;
		waitms(100);
				ISR_pwm1=130;
		waitms(100);
				ISR_pwm1=140;
		waitms(100);
				ISR_pwm1=150;
		waitms(100);
				ISR_pwm1=160;
		waitms(100);
				ISR_pwm1=170;
		waitms(100);
				ISR_pwm1=180;
		waitms(100);
		ISR_pwm1=180;
		waitms(200);

		//bring ISR_pwm1 from 200 to 180
		//for(i=0;i<9;i++){
		//ISR_pwm1=200-2*i;
		//waitms(50);
		//}
		ISR_pwm1=160;
		waitms(1000);

	
		//bring ISR_pwm2 from 90 to 180 with delays(raise arm)
		 
		for(i=0;i<=15;i++){
		ISR_pwm2=90+i*8;
		waitms(100);
		
		}
		
		
		waitms(1000);

		
		//bring ISR_pwm1 from 160 to 240(pivot arm to drop coin in bucket)
		for(i=0;i<20;i++){
		ISR_pwm1=160+i*4;
		waitms(100);
		}
		
		ISR_pwm1=240;
		waitms(1000);

		//turn magnet off
		LATBbits.LATB15 = 0;
		waitms(1000);

		for(i=1;i<=7;i++){
		ISR_pwm1=240-i*20;
		waitms(100);
		}


		ISR_pwm1=95;
		waitms(1000);

		ISR_pwm2=240;
		waitms(1000);


}



//DETECTION ROUTINES


int detect_metal(float average){
	float count,f;
	
		count=GetPeriod(100);
		f=(count*2.0)/(SYSCLK*100.0);
		f=1.0/f;
		//PrintNumber(f, 10, 3);
		//uart_puts("\r");
		waitms(100);
		//frequency found
		if(f>58500){
		return 1;
		}
		else{
		return 0;
		}

}

//NOTE PREIMTETER DETECTION ADC PINS ARE USED FOR JOYSTICKS IN THIS CODE

int detect_perimeter(){
float vmax,voltage,vavg;
int perim1,perim2,i;	
		//read peak value from first peak detector
		vmax=0;
		for(i=0;i<100;i++){
		voltage=ADCRead(4)*3.3/1023.0;
			if(voltage>vmax){
				vmax=voltage;
			}
		}

		//printf("%f \r",vmax);
		//now vmax stores maximum voltage, use it to trigger flag
		if(vmax>0.3){
			perim1=1;
		}
		else{
			perim1=0;
		}
			
		//read peak value from second peak detector
		vmax=0;
		vavg=0;
		for(i=0;i<100;i++){
		voltage=ADCRead(5)*3.3/1023.0;
		vavg=vavg+voltage;

			if(voltage>vmax){
				vmax=voltage;
			}
		}


		//PrintNumber(vmax, 10, 3);

	
	
		//now vmax stores maximum voltage, use it to trigger flag
		if(vmax>0.5){
			perim2=1;
		}
		else{
			perim2=0;
		}
		
		//trigger LED if either perimiter detector works
		if((perim1||perim2)==1){
			return 1;

		}
		else{
			return 0;
		}


}

//RANDOM NUMBER GENERATOR FUNCTION

int random_time(seed,n){

		int temp;
		temp=124859+7919*((seed*4999)%5)*((n*8278)%5209);
		temp=temp%1999;
		
		if(temp<1000){
			temp=temp+1000;
		}
		

}



void main(void)
{	
	float joystick2_x,joystick2_y=2.5;
	float joystick1_x,joystick1_y=2.5;
	int mode=0;
	int counter=0;
	int n=0;
	//initialize magnet as off
	LATBbits.LATB15 = 0;
	//TRISBbits.TRISB6 = 0; //pin6 is output for LED
	//TRISBbits.TRISB4 = 0; //pin4 is output for LED
	float average;
	int perim1,perim2,perim,i,metal;
	float voltage,vmax;
	volatile unsigned long t=0;
    int adcval;
    long int v;
	unsigned long int count, f;
	unsigned char LED_toggle=0;
	
	CFGCON = 0;
  
    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
    ConfigurePins();
    SetupTimer1();
  
    ADCConf(); // Configure ADC
    
    //TRISBbits.TRISB6 = 0;
	//LATBbits.LATB6 = 0;	
    
    waitms(500); // Give PuTTY time to start (PuTTY is a serial console used for debugging)
	uart_puts("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	uart_puts("\r\nPIC32 multi I/O example.\r\n");
	uart_puts("Measures the voltage at channels 4 and 5 (pins 6 and 7 of DIP28 package)\r\n");
	uart_puts("Measures period on RB5 (pin 14 of DIP28 package)\r\n");
	uart_puts("Toggles RA0, RA1, RB0, RB1, RA2 (pins 2, 3, 4, 5, 9, of DIP28 package)\r\n");
	uart_puts("Generates Servo PWM signals at RA3, RB4 (pins 10, 11 of DIP28 package)\r\n\r\n");
	
	//for metal detector self calibration
	average=0;
	for(i=0;i<10;i++){
	//count=GetPeriod(100);
	f=(count*2.0)/(SYSCLK*100.0);
	f=1.0/f;
	average+=f;
	}
	average=average/10.0;
	
	
	ISR_pwm1=95;
	ISR_pwm2=240;
	while(1)
	{
	
	if(counter==20){
		break;
	}
	
	

	

//this if statement allows us to easily ignore the follwing code to enter manual control mode, mode=0
if(mode==1){
	
	
	/*

//move forwards by default
	move_forwards();


//respond to metal
	if(detect_metal(1)){
		counter++;
		stop();
		waitms(100);
		move_backwards(); //move to position of metal
		waitms(400);
		stop();
		waitms(100);
		pick_up_coin(); //grab the coin once in position
		waitms(100);
	}
	
	move_forwards(); //move forwards by defualt
		
	if(detect_perimeter()){
		move_backwards(); //back up to not pass the perimeter when turning
		waitms(500);
		turn_right(); //change trajectory
		waitms(random_time(7,n));
		stop();
		n++;
		
	}	
	
	move_forwards(); //move forwards by defualt
	
	*/

	}


	


	//THIS IS WHERE MANUAL CONTROL EXISTS

	else{
	

	//MOVING ARM UP


	joystick2_y=ADCRead(11)*3.3/1023.0;
	PrintNumber(joystick2_y, 10, 3);
	uart_puts("\r");
	if(	joystick2_y>2.4){
				joystick2_y=ADCRead(11)*3.3/1023.0;
			
			//set limit lower than theshold so that there isn't jittering from noise
		while(	joystick2_y>2.4-0.3){
			joystick2_y=ADCRead(11)*3.3/1023.0;
				PrintNumber(joystick2_y, 10, 3);

		uart_puts("moving up");
					PrintNumber(ISR_pwm2, 10, 3);
			uart_puts("\r");
			//travel up at speed such that arm goes from bottom to top in 2 seconds
			if(ISR_pwm2<240){
				//move servo arm up
				ISR_pwm2+=5;
				waitms(65);	
			}
		}
	} 

	//MOVING ARM DOWN
	
		joystick2_y=ADCRead(11)*3.3/1023.0;
		//vertical downwards movement
	if(	joystick2_y<0.6){
				joystick2_y=ADCRead(11)*3.3/1023.0;
			//set limit greater than theshold so that there isn't jittering from noise
		while(joystick2_y<0.6+0.3){
				joystick2_y=ADCRead(11)*3.3/1023.0;
			PrintNumber(joystick2_y, 10, 3);
			

			uart_puts("moving down");
			PrintNumber(ISR_pwm2, 10, 3);
			uart_puts("\r");
			//travel down at speed such that arm goes from top to bottom in 2 seconds
			if(ISR_pwm2>90){
				//move servo arm down
				ISR_pwm2-=5;
				waitms(65);	
			}
		}
	} 
	
	
	
	}

	//PIVOTING ARM TO LEFT

	joystick2_x=ADCRead(12)*3.3/1023.0;
	
	//horizontal left movement
	if(joystick2_x>2.4){
			//set limit lower than theshold so that there isn't jittering from noise
		while(joystick2_x>2.1){
			joystick2_x=ADCRead(12)*3.3/1023.0;
			//travel left at speed such that arm goes from left to right (60 to 240) in 2s
			if(joystick2_x<240){
				//pivot base of arm to left
				ISR_pwm1+=5;
				waitms(55);	
			}
		}
	} 
	
	

	//PIVOTING ARM TO RIGHT


	joystick2_x=ADCRead(12)*3.3/1023.0;
	//horizontal right movement
	if(joystick2_x<0.6){
			//set limit greater than theshold so that there isn't jittering from noise
		while(joystick2_x<0.9){
		joystick2_x=ADCRead(12)*3.3/1023.0;
			//travel right at speed such that arm goes from right to left (240 to 60) in 2s
			if(ISR_pwm1>60){
				//pivot base of arm to rght
				ISR_pwm1-=5;
				waitms(55);	
			}
		}
	}
	
	//initialize wheels with stop
	
		stop();
	
	
	//BACKWARDS MOVEMENT
	
	
	 
	joystick1_x=ADCRead(4)*3.3/1023.0;
	if(joystick1_x<0.6){
			//set limit greater than theshold so that there isn't jittering from noise
		while(joystick1_x<0.9){
	joystick1_x=ADCRead(4)*3.3/1023.0;
		
			//move backwards
			move_backwards();
			
				
			}
		}
	
	
	stop();
	
	
	//FORWARDS MOVEMENT

		joystick1_x=ADCRead(4)*3.3/1023.0;
	if(joystick1_x>2.4){
			//set limit lower than theshold so that there isn't jittering from noise
		while(joystick1_x>2.1){
			joystick1_x=ADCRead(4)*3.3/1023.0;
		
			move_forwards();
			}
		}
	
	stop();
	
	//LEFT TURN
	
		joystick1_y=ADCRead(5)*3.3/1023.0;
	//turn left
	if(joystick1_y<0.6){
			//set limit greater than theshold so that there isn't jittering from noise
		while(joystick1_y<0.9){
	joystick1_y=ADCRead(5)*3.3/1023.0;
		
			//move backwards
			turn_left();
			
				
			}
		}
	
	
	stop();
	
	
	//RIGHT TURN

		joystick1_y=ADCRead(5)*3.3/1023.0;
	
	//horizontal left movement
	if(joystick1_y>2.4){
			//set limit lower than theshold so that there isn't jittering from noise
		while(joystick1_y>2.1){
			joystick1_y=ADCRead(5)*3.3/1023.0;
			
			turn_right();
			}
		}
	
	stop();
	


	//end of main while loop
	}



	stop();




	
	
	
//end of main
}

