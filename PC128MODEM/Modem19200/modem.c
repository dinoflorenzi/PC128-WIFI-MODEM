#include "mo5_lib.h"
#include "cmoc.h"
#define BUFFERRX_SIZE 255
#define BUFFERTX_SIZE 128
#define NCOMM 7
#define MAXERR 100
#define debug *((char*)0xafff)=0

char ch;
unsigned short rxPos,dwSize,delvar=500;
unsigned char ret_cts;
char * buffer;
//char * bufferRx=(char*) 0x6000;




 char  oldTx[BUFFERTX_SIZE];
 char RXbuffer[BUFFERRX_SIZE];
char esc=0;
unsigned short index = 0;
 char bufferTx[BUFFERTX_SIZE+1];

char *commlist[]={"upload","download","type","delay","basic","loadbas","loadimg"};
char *commusage[]={"Nomefile Start Size Exec Bank\n\r","FILE/FILE START BANK\n\r","Nomefile\n\r","uSec/10\n\r","Nomefile\n\r","Nomefile\n\r"};
char commnpar[]={5,1,1,1,0,1,1} ;
void (*commptr[])(char**) ={upload,download,type,setdelay,basic,loadbas,loadgame};
void upload(char **);
void download(char **);
void basic(char **);
void setdelay(char **);
void type(char**);
void loadbas(char**);
void loadgame(char**);
void printBuffer(char *, unsigned short);

void basic(char ** params)
{
	asm
	{
		RTS
	}
}


void delay(unsigned short n)	// 	n * 10 uSec
{
	if(n==0)return;
	asm
	{
		PSHS X,CC
		ORCC #$50
		LDX n
LOOP_DELAY
		NOP
		LEAX -1,X
		BNE LOOP_DELAY
		PULS X,CC
	}
}
void setdelay(char ** params)
{
	
	delvar = atoi(params[1]);
	printf("%d\n",delvar);
	
}

char chkscalc(char* data, unsigned short length) {
    unsigned short checksum = 0;
    for (unsigned short i = 0; i < length; i++) {
        checksum += data[i];
    }
    return ( char )(checksum & 0xFF);
}

char presskey()
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
		ORA #$10
		STA $A7CC
	}
}

void check_CTS()
{
	unsigned char ret_cts;
	unsigned short timeout=1000;
	while(1)
	{
			ret_cts=CTS();
			if((ret_cts==0x20) || (timeout==0)) break;
			timeout--;
	}
}

char setbank(char nbank)
{
	char* addr=(char*)0xa7e5;
	char bank=*addr;
	*addr=(nbank & 0x1f)|(bank&0xe0);
	return bank;
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
void serial_tx(char* buf,unsigned short len )
{
	
	asm
	{
	 PSHS X,Y,D
	 LDX buf
	 LDY len
	 ORCC #$50
LOOP2_TX
	 CMPY #0
	 BEQ EXIT_TX
	 LDA #$00
	 STA $A7CC
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 LDB #8
	 LDA ,X+
	 LEAY -1,Y
LOOP_TX
	 PSHS A
	 ANDA #$01
	 BEQ LOW
	 BSR SETHIGH_TX
	 BRA RET
LOW
	 BSR SETLOW_TX
RET
	 PULS A
	 LSRA
	 DECB
	 BNE LOOP_TX
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 BSR SETHIGH_TX
	 LDA #4
RIT_TX
	 DECA
	 BNE RIT_TX
	 BRA LOOP2_TX
SETHIGH_TX
	 LDA #$80
	 STA $A7CC
	 NOP
	 NOP
	 NOP
	 RTS
SETLOW_TX
	 LDA #$00
	 STA $A7CC
	 BRN SETLOW_TX
	 NOP
	 NOP
	 NOP
	 RTS
EXIT_TX
	 PULS X,Y,D
	}
	return ;
}
unsigned short serial_rx(char* buf, unsigned short len)
{
	char* res;
	asm
	{
	 PSHS X,Y,D
	 ORCC #$50
	 LDX buf
	 LDD len
	 LEAY D,X
	 PSHS Y
START
	 LDY #$02FF
LOOP
	 LDA $A7CC     
	 ANDA #$40
	 BEQ EXIT
	 CMPX ,S
	 BEQ EXIT2
	 LDA $A7CC     
	 ANDA #$40
	 BEQ EXIT
	 LEAY -1,Y
	 BNE LOOP
	 BRA EXIT2
EXIT
	 BRN RIT
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 LDB #8
RIT
	 LSR ,X
	 LDA $A7CC
	 ANDA #$40
	 BEQ SETLOW
	 LDA ,X
	 ORA #$80
	 BRA SETHIGH
SETLOW
	 BRN SETHIGH
	 LDA ,X
	 ANDA #$7F       
SETHIGH
	 STA ,X
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 NOP
	 DECB
	 BNE RIT
	 LEAX 1,X
	 BRA START
EXIT2
	 STX res
	 PULS X
	 PULS X,Y,D
	}
		unsigned short size=(unsigned short) (res-buf);

	return size;
}

void upCase2Low(char ** str)
{
	char* pstr=*str;
	int i=0;
	while(1)
	{
		if(pstr[i]==0)break;
		if((pstr[i]>64) && (pstr[i]<91))
			pstr[i]+=32;
		i++;
	}
}

int scan( char * comm)
{
	for(int i=0;i<NCOMM;i++)
	{
		//printf("%s%s\n",comm,commlist[i]);
		if(strcmp(comm,commlist[i])==0)
			return i;
	}
	
	return -1;
}

void strsplit(char * comm)
{ 
	if(comm==NULL)return;

	int npar=0;
	int ncom=-1;
	char * params[5]={NULL,NULL,NULL,NULL,NULL};
	params[npar]=comm;
    int length=strlen(comm);
	for(int i=0;i<length;i++)
	{
		if(comm[i]==32)
		{ 
			npar++;
			comm[i]='\0';
			params[npar]=&comm[i+1];
			
			if(npar==1)
			{
				upCase2Low(&params[0]);
				ncom=scan(params[0]); 	
		}

		}
		if(npar==7)break;
	}
	if(ncom==-1)return; 
	
	if(npar<commnpar[ncom])
	{
		PRINT(commusage[ncom]);
	}
	else
	{
		commptr[ncom](params);
		debug;
	}
}


void upload(char ** params)
{
	
  unsigned char err=0;
unsigned short blksize;
	char * start =(char*) atoi(params[2]);
	unsigned short size =(unsigned short) atoi(params[3]);
	char nbank =(char) atoi(params[5]);
	char oldbank = setbank(nbank);
  unsigned short nblk=size / BUFFERTX_SIZE;
unsigned short rest = size - nblk * BUFFERTX_SIZE;

nblk++;

		
      while(err<MAXERR)
      {
	  check_CTS();
      RTS_ON();
      serial_rx(RXbuffer,5);
	  RTS_OFF();
	  RXbuffer[4]='\0';
      if(strcmp(RXbuffer,"STT\n")==0) break;
	  else err++;
      }
   
	  printf("%d blocchi\n\r",nblk);
	  err=0;

unsigned short i=0;
while(i<nblk&&err<MAXERR)
{
      if(i<nblk-1)
      blksize = BUFFERTX_SIZE;
      else
      blksize = rest;
      char chks = chkscalc(start,blksize);
      serial_tx(start,blksize);
      serial_tx(&chks,1);
	  while(err<MAXERR)
      {
      check_CTS();
      RTS_ON();
      serial_rx(RXbuffer,5);
      RTS_OFF();
	  RXbuffer[4]='\0';
      if(strcmp( RXbuffer,"ACK\n")==0) {err=0;i++; break;}
      if(strcmp(RXbuffer,"NAK\n")==0) {err++; break;}
	  err++;
	  //PUTCH('.');
      }
      start +=blksize;
}
	setbank(oldbank);
	if(err==MAXERR)printf("errore di trasmissione\n\r");
}

void download(char ** params)
{
		char * start =(char *) atoi(params[2]);
		dwSize=atoi(params[3]);	
		char bank =(char)  atoi(params[4]);
		char oldbank=setbank(bank);
		unsigned short total=0,count=0,count2=0,timeout=0;
		char ACK=0x06;
		char NAK=0x15;
		char CNT=0x0a;
		char err=0;
		char chkr;
	while((total<dwSize)&&(err<40)&&(timeout<100))
	{
	  delay(delvar);
	  if(count==0)
	  {
		  check_CTS();
		  RTS_ON();
		  count= serial_rx(start,128);
		  RTS_OFF();
		  serial_tx(&CNT,1);
	  }
	  
	if(count2==0 && count>0)
	  {

		check_CTS();
		RTS_ON();
		count2= serial_rx(&chkr,1);
		RTS_OFF();
	  }
	  if(count>0 && count2>0)
	  {
		  char chks = chkscalc(start,count);

	  	  //printf("%d %d-",chks,chkr);
		  delay(delvar);
		  if(chks==chkr)
		  {
			start+=count;
			total+=count;
			serial_tx(&ACK,1);

		  }
		  else
		  {
			serial_tx(&NAK,1);  
			err++;
		  }
		  count=0;
		  count2=0;
	  }
	  else timeout++;
	}
	setbank(oldbank);
	if(err==40)printf("errore di ricezione\n\r");
	if(timeout==100)printf("Timeout!!!!\n\r");

}
void loadbas(char ** params)
{

		dwSize=0x4000;	
		char oldbank=setbank(2);
		unsigned short total=0,timeout=0,count=0,count2=0;
		char ACK=0x06;
		char NAK=0x15;
		char CNT=0x0a;
		char err=0;
		char chkr;
	for(unsigned short i=2;i<8;i++)
	{
	setbank(i);
	char * start =(char *) 0x6000;
	while((total<dwSize)&&(err<40))
	{
	  delay(delvar);
	  if(count==0)
	  {
		  check_CTS();
		  RTS_ON();
		  count= serial_rx(start,128);
		  RTS_OFF();  
		  serial_tx(&CNT,1);
	  }
	  if(count2==0 && count>0)
	  {
		  check_CTS();
		  RTS_ON();
		  count2= serial_rx(&chkr,1);
		  RTS_OFF();  
	  }

	  if(count>0 && count2>0)
	  {
		  timeout=0;
		  char chks = chkscalc(start,count);
		  delay(delvar);
		  if(chks==chkr)
		  {
			start+=count;
			total+=count;
			serial_tx(&ACK,1);
		  }
		  else
		  {
			serial_tx(&NAK,1);  
			err++;
		  }
		count=0;
		count2=0;
	  }
	  else timeout++;
	  if(timeout>100)break;
	}
	  if(timeout>100)break;
	}
		setbank(oldbank);
	if(err==40)printf("errore di ricezione\n\r");

}
void type(char ** params)
{ 

		unsigned short timeout=0,count =0, count2=0;
		char ACK=0x06;
		char NAK=0x15;
		char CNT=0x0a;
		char err=0;
		char chkr;
	while(err<40 && timeout<100)
	{
		if(count==0)
		{
			check_CTS();
			RTS_ON();
			count= serial_rx(RXbuffer,128);
			RTS_OFF();
			serial_tx(&CNT,1);	
					
		}
		if(count2==0 && count>0)
		{
			check_CTS();
			RTS_ON();
			count2= serial_rx(&chkr,1);
			RTS_OFF();
		}
	  if(count>0 && count2>0)
	  {
		printBuffer(RXbuffer,count);
		serial_tx(&ACK,1); 
		timeout=0;
		count =0;
		count2 =0;
	  }
	  else timeout++;
	if(timeout==40) break;
		
	}
	if(err==40 || timeout==100)printf("errore di ricezione\n\r");
}

void CTS_ASM()
{
	asm
	{
	LDX #$FFFF
LOOP4
	LEAX -1,X
	BEQ EXIT3
	LDA $A7CC
	ANDA #$20
	BEQ LOOP4
EXIT3
	}
}
void dummyTX()
{
	asm
	{
	LDA #$00
	STA $A7CC
	LDB #10
LOOP3
	DECB
	BNE LOOP3
	LDA #$80
	STA $A7CC
	}
}

void loadgame(char ** params)
{
	delay(500000);
	asm
	{
		ORCC #$50
		LDS #$1FFF
		LDA #0
		STA $A7E5
		LDA $A7C0
		ORA #$01
		STA $A7C0
		LDA #$10
		STA $A7DD
		JMP $7600,PC
	}
	
	CTS_ASM();
	RTS_ON();
	serial_rx(0x1f40,23);     //cc,d,dp,x,y,u,pc
	RTS_OFF();
	
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16192);
	RTS_OFF();
	asm
	{
		LDA #1
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);
	RTS_OFF();
	asm
	{
		LDA #2
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);
	RTS_OFF();
	asm
	{
		LDA #3
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);	
	RTS_OFF();
	asm
	{
		LDA #4
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);
	RTS_OFF();
	asm
	{
		LDA #5
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);
	RTS_OFF();
	asm
	{
		LDA #6
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);
	RTS_OFF();
	asm
	{
		LDA #7
		STA $A7E5
	}
	dummyTX();
	CTS_ASM();
	RTS_ON();
	serial_rx(24576,16384);
	RTS_OFF();
	
	asm
	{
	LDD $1F4B
	ORA #$01
	STA $A7C0
	STB $A7E5
	LDX $1F4F
	LDY $1F51
	LDS $1F53
	LDU $1F55
	LDD $1F4D
	TFR A,DP
	TFR B,CC
	STA $AFFF
	JMP $1F40
	}
}

void printBuffer(char *bufferRx, unsigned short size )
{
	if(size>0)
		{
			irq_dis();
			for(unsigned short i=0;i<size;i++)
			{
				if(i>BUFFERRX_SIZE) 
				{
					break;
				}
				ch=bufferRx[i];
				if(ch==10)PUTCH(13);
				if(ch==31) ch=0;
				if(ch==27)
				{
					ch=0;
					if(bufferRx[i+1]==0x5b && bufferRx[i+2]==0x48)
					{
						ch=0x1e;
						i+=2;
					}
					if(bufferRx[i+1]==0x5b && bufferRx[i+2]==0x32 && bufferRx[i+3]==0x4a )
					{
						ch=12;
						i+=3;
					}	
				}
				PUTCH(ch);
			}

		}
}
// RTS LOW to receive data - CTS HIGH
// RTS HIGH to send data - wait CTS LOW
int main()
{	
	PUTCH(17);
	PUTCH(12);
	PRINT("PC128 WIFI MODEM 19200 BAUD 1.3");
	PUTCH(10);
	PUTCH(13);
	serial_init();
	RTS_OFF();
	while(1)
	{

		printBuffer(RXbuffer,rxPos);
		rxPos=0;
		irq_en();		
		ch=GETCH();
		if(ch!=0)
		{
		
			if(ch==13)
			{
				PUTCH(13);
				PUTCH(10);
				serial_tx(&ch,1);
				bufferTx[index]='\0';
				//strcpy(oldTx,bufferTx);
				strsplit(bufferTx);
				index=0;
			}
			else
			{
			PUTCH(ch);
			serial_tx(&ch,1);
			if(index<BUFFERTX_SIZE)
				bufferTx[index++]=ch;
			}
		}

			
		ret_cts=CTS();
			if(ret_cts==0x20)
			{	
				RTS_ON();
				rxPos= serial_rx(RXbuffer,BUFFERRX_SIZE);
				RTS_OFF();
			}
			

	}

	return 0;
}
