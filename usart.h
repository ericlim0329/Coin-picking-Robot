#define _BAUD 115200UL
#define FOSC 16000000UL
#define _UBRR (FOSC/8)/_BAUD - 1 // Used for UBRRL and UBRRH
#define _DATA 0x03 // Number of data bits in frame = byte tranmission
#define TX_START() UCSR0B |= _BV(TXEN0)	 // Enable TX
#define TX_STOP()  UCSR0B &= ~_BV(TXEN0) // Disable TX
#define RX_START() UCSR0B |= _BV(RXEN0)	 // Enable RX
#define RX_STOP()  UCSR0B &= ~_BV(RXEN0) // Disable RX

void usart_init(void);
char usart_getchar( void );
void usart_putchar( char data );
void usart_pstr(char *s);
unsigned char usart_kbhit(void);
int usart_putchar_printf(char var, FILE *stream);
