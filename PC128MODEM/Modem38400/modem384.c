//#include "mo5_lib.h"
#include "cmoc.h"
#define BUFFERRX_SIZE 4096
#define BUFFERTX_SIZE 256
#define NCOMM 2
#define POKE(addr,val)     (*(unsigned char*) (addr) = (val))
#define POKEW(addr,val)    (*(unsigned*) (addr) = (val))
#define PEEK(addr)         (*(unsigned char*) (addr))
#define PEEKW(addr) (*(unsigned*) (addr))
unsigned char ch;
unsigned short rxPos;
unsigned char ret_cts;
char * buffer;
unsigned char bufferRx[BUFFERRX_SIZE];
unsigned char esc=0,index=0;
unsigned char bufferTx[BUFFERTX_SIZE];

char *commlist[]={"upload","download"};
char *commusage[]={"Start Size Exec Bank","NONE/START BANK"};
char commnpar[]={5,0};
void *commptr[]={upload,download};


unsigned char GETCH(void)
{
	unsigned char res;
	asm
	{
		swi
		.byte 10
		stb res
	}
	return res;	
}

unsigned char presskey()
{
	ch=0;
	while(1)
	{
		ch=GETCH();
		if(ch!=0)break;
	}
	return ch;
}
void irq_en()
{
	asm
	{
		ANDCC #$AF
	}
}
void irq_dis()
{
	asm
	{
		ORCC #$50
	}
}

void color()
{
	asm
	{
		lda $a7c0
		anda #$fe
		sta $a7c0
	}
}
void matrix()
{
	asm
	{
		lda $a7c0
		ora #$01
		sta $a7c0
	}
}
unsigned int hex2int(char *hex) {
    unsigned int val = 0;
    while (*hex) {
        // get current character then increment
        unsigned char byte = *hex++; 
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}
void serial_init()
{
	// WEMOS CTS GPIO5 D1 -> DB9-1 PA4 RTS OUTPUT
	// WEMOS RTS GPIO4 D2 -> DB9-2 PA5 CTS INPUT
	// WEMOS TX	 GPIO1 TX -> DB9-3 PA6 RX INPUT
	// WEMOS RX	 GPIO3 RX -> DB9-4 PA7 TX OUTPUT
	asm
	{
	LDA $A7CE
	ANDA #$FB
	STA $A7CE 
	LDA #$90
	STA $A7CC
	LDA $A7CE
	ORA #$04
	STA $A7CE
	}
}

unsigned char CTS()
{
	unsigned char res=0;
	asm
	{
		LDA $A7CC
		ANDA #$20
		STA res
	}
	return res;
}

void RTS_ON()
{
	asm
	{
		LDA $A7CC
		ORA #$ff
		STA $A7CC
	}
}

void check_CTS()
{
	unsigned char ret_cts;
	while(1)
	{
			ret_cts=CTS();
			if(ret_cts==0x20) break;
	}
}
void RTS_OFF()
{
	asm
	{
		LDA $A7CC
		ANDA #$EF
		STA $A7CC
	}
}

void setbank(unsigned char nbank)
{
	unsigned char* addr=0xa7e5;
	*addr=nbank & 0x1F;
}

void serial_tx(unsigned char* buf,unsigned short len )
{
	
	asm
	{
	 pshs x,y,d
	 ldx buf
	 ldy len
	 orcc #$50
	 ldb #8
	 pshs b        
     lda ,x+
	 andcc #$fe
loop_tx
	 rora
	 tfr a,b
	 andb #$80
	 orb #$00
	 stb $a7cc
	 dec ,s
	 bne loop_tx
	 rora
	 anda #$80
	 ora #$00
	 nop
	 nop
	 nop
	 sta $a7cc
	 ldd #$08ff
	 sta ,s
	 andcc #$fe
	 lda ,x+
	 brn exit_tx
	 ldb #$80
	 stb $a7cc
	 leay ,-y
	 bne loop_tx
exit_tx
	 puls b
	 andcc #$af
	 puls x,y,d
	}
}
unsigned short serial_rx(unsigned char* buf)
{
	unsigned char* res =0;
	asm
	{
	 pshs x,y,d
	 orcc #$50
	 ldx buf  ;start receive mem address
start
	 ldy #$1fff      ;4
loop
	 lda $a7cc     ;5 
	 anda #$40   ;2
	 beq exit_rx   ,3
	 leay -1,y        ;4
	 bne loop      ;3
	 bra exit2       ;3
exit_rx
	 ldb #8           ; 2
	 ldy #$1fff      ;4
	 leax 1,x      ; 4
	 nop             ;2
	 nop             ;2
	 nop             ;2
rit
	 lda  $a7cc      ; 5 read port
	 lsla            ; 2
	 lsla            ; 2
	 ror  -1,x         ; 6 shift in bit
	 nop ; 2
	 nop ;2
	 nop                 ; 2
	 decb                ; 2
	 bne     rit         ; 3   / 26
	 bra loop    ; 3
exit2
	 stx res
	 andcc #$af
	 puls x,y,d
	}

	return res-buf;
}

void PUTCHAR40(unsigned char ch)
{
	asm
	{
	pshs d,x,y
	ldb $201b		;load current y cursor
	lda $a7c0
	pshs a
	anda #$df
	sta $a7c0		;switch to char memory map
	lda #160
	mul
	lslb
	rola
	tfr d,x			;calc video y memory position
	ldb $201c		;load current x cursor
	decb
	abx				;add x cursor to video y memory
	inc $201c		;inc x cursor position
	cmpb #39		;check if right limit reached
	bne skip40		;if no skip
	lda #1			;reset cursor position to x=1,y=y+1
	sta $201c
	inc $201b
skip40
	ldy #$f1c2		;load char map pointer
	lda ch			;load asc value
	ldb #8			;load register with q.ty byte of char definition
	suba #32		;sub 32 to asc value 
	mul				;add offset of char to char map y pointer
	leay d,y
	ldb #8			;load register with q.ty byte of char definition
loop40				;loop to get the 8 char bytes
	lda b,y
	sta ,x			;print char byte to video
	leax 40,x		;jump to next video line
	decb			;decement the 8 loop iteration
	bne loop40
	puls a			;restore and switch to 
	sta $a7c0		;previous memory map
	puls d,x,y		;restore registers
	}
}


void PUTCHAR80(unsigned char ch)
{
	asm
	{
	pshs d,x,y
	ldb $201b
	lda $a7c0
	pshs a
	anda #$df
	sta $a7c0
	lda #160
	mul
	lslb
	rola
	tfr d,x
	ldb $201c
	decb
	lsrb
	abx
	ldb $201c
	inc $201c
	decb
	andb #01
	comb
	andb $a7c0
	stb $a7c0
	ldy #$f1c2
	lda ch
	ldb #8
	suba #32
	mul
	leay d,y
	ldb #8
loop80
	lda b,y
	sta ,x
	leax 40,x
	decb
	bne loop80
	ldb $201c
	cmpb #81
	bne skip80
	lda #1
	sta $201c
	inc $201b
skip80
	puls a
	sta $a7c0
	puls d,x,y
	}
}

void PUTCH(unsigned char ch)
{
	asm
	{
		ldb ch
		swi
		.byte 2
	}
	if(ch==12)matrix();
}

void gotoxy(unsigned char xx, unsigned char yy)
{
	PUTCH(0x1F);
	PUTCH(0x40+yy);
	PUTCH(0x41+xx);
}	


void FPUTCH(unsigned char ch)
{
	unsigned short limX;
	if(PEEK(0x2019)==4)
		limX=0x1828;
	else
		limX=0x1850;
	
	if ((ch<32) || (ch>127)||(PEEKW(0x201b)==limX))
	{
		//POKE(0xa7fe,0);
		gotoxy(PEEK(0x201c)-1, PEEK(0x201b));
		PUTCH(ch);
		matrix();
	}

else
{
		if(limX==0x1828) 	
		PUTCHAR40(ch);
	else
		//if(PEEK(0x2019)==84)
		PUTCHAR80(ch);
}
}
void PRINT(char * str)
{
	unsigned char i = 0;
	
	while(str[i]!='\0')
	{
		PUTCH(str[i]);
		++i;
	}
}
void go40()
{
    PUTCH(27);
    PUTCH(124);
	matrix();
}

void go80()
{
    PUTCH(27);
    PUTCH(125);
	matrix();
}

void setPaletteBGR(unsigned char c,unsigned short bgr)
{
	asm
	{
		pshs d,y
		lda c
		lsla
		sta $a7db
		ldd bgr
		stb $a7da
		sta $a7da
		puls d,y
	}
}
int scan(unsigned char * comm)
{
	for(int i=0;i<NCOMM;i++)
	{
		if(strcmp((comm[0]),commlist[i])==0)
			return i;
	}
	return -1;
}
int strlen2(char *str)
{
	int i=0;
	while(1)
	{
		if(str[i++]==0)
			return i;
	}
}
void strsplit(char * comm)
{ 

	if(comm==NULL)return;

	int npar=0;
	int ncom=-1;
	char * params[5]={NULL,NULL,NULL,NULL,NULL};
	params[0]=comm;

	for(int i=0;i<strlen(comm);i++)
	{
		if(comm[i]==32)
		{

			comm[i]=0;
			
			params[npar++]=&comm[i+1];
			
			if(npar==1)
			{
				ncom=scan(params[0]);
				if(ncom==-1)return;
			}

		}
		if(npar==5)break;
	}
	if(ncom==-1)return; 
	
	if(npar!=commnpar[ncom])
	{
		PRINT(commusage[ncom]);
	}
	else
	{
		void(*command)(char**);
		command = (void*)commptr[ncom];
		command(params);
	}
}


void upload(char ** params)
{
	unsigned short start = atoi(params[1]);
	unsigned short size = atoi(params[2]);
	unsigned char nbank = atoi(params[4]);
	setbank(nbank);
	serial_tx(start,size);
}

void download(char ** params)
{
	
}
// RTS LOW to receive data - CTS HIGH
// RTS HIGH to send data - wait CTS LOW
int main()
{	
	setPaletteBGR(0,0xff0);
	setPaletteBGR(1,0xf00);
	go80();
	PRINT("PC128 WIFI MODEM 38400 BAUD 1.0");
	PUTCH(10);
	PUTCH(13);
	serial_init();
	RTS_OFF();

	//printf("%x\n\r",bufferRx);
	//POKE(0xa7ff,0);
	while(1)
	{
		//rxPos=16000;   //test
		if(rxPos>0)
		{

			irq_dis();
			for(unsigned short i=0;i<rxPos;i++)
			{

				ch=bufferRx[i];
				//POKE(0xa7ff,ch);
				if(ch==10)PUTCH(13);
				if(ch==31) ch=0;

				if (esc==2)
				{
				if(ch==0x48) 
					{
						esc=0;
						ch=0x1e;
					} else
					if (ch==0x32) esc++;
					else esc=0;
				}


				if(esc==3 && ch==0x4a )
				{
					ch=12;
					esc=0;
				}
				
				if (esc==1&&ch==0x5b) 
					esc++;
				else 
					esc=0;
				
				if(ch==27) esc=1;
				if(esc==0&&ch!=0) FPUTCH(ch);
			}

			rxPos=0;
			irq_en();
		}		

		ch=GETCH();

		
		if(ch!=0)
		{

			switch(ch)
			{
				case 193:
				go40();
				break;
				case 194:
				go80();
				break;
				case 195:
				RTS_ON();
				PRINT("RTS ON");
				PUTCH(10);
				PUTCH(13);
				break;
				case 196:
				RTS_OFF();
				PRINT("RTS OFF");
				PUTCH(10);
				PUTCH(13);
				break;
				case 10:
				case 13:
				PUTCH(13);
				PUTCH(10);
				ch=13;
				serial_tx(&ch,1);
				ch=10;
				serial_tx(&ch,1);
				bufferTx[index]='\0';
				strsplit(bufferTx);
				index=0;
				break;
				default:
				PUTCH(ch);
				serial_tx(&ch,1);
				if(index<BUFFERTX_SIZE)
				bufferTx[index++]=ch;
				break;
			}


		}

			ret_cts=CTS();
			if(ret_cts==0x20)
			{	
				RTS_ON();
				rxPos= serial_rx(bufferRx);
				RTS_OFF();
			}
			

	}

	return 0;
}
