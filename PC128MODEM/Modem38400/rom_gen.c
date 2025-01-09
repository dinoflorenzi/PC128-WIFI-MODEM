#include<stdio.h>
#include<string.h>
unsigned short swapen(unsigned short addr)
{
	unsigned short bh=(unsigned short)((addr<<8)&0xff00);
	unsigned short bl=(unsigned short)((addr>>8)&0x00ff);
	return bh|bl;
}

int main(int argc,char *argv[])
{
		unsigned char code[]={ 0x10 ,0xCE ,0x39 ,0xFF ,0x8E ,0x3a ,0x00 ,0xA6 ,0x89 ,0x76 ,0x00 ,0xA7 ,0x80 ,0x8C ,0x60 ,0x00 ,0x26 ,0xF5 ,0x7E ,0x3a ,0x00};
		unsigned short block_size;
		unsigned short block_offset;
		unsigned char byte;
		FILE *in=NULL;
		FILE *out=NULL;
		char buffer[0x4000];
		char *title=argv[2];
		memset(buffer,0,0x4000);
		in=fopen(argv[1],"rb");
		if(in==NULL||title==NULL)
		{
			printf("errore nei parametri\n");
		return 1;	
		}
		fseek(in,0L,SEEK_END);
		int size=ftell(in);
		fseek(in,3L,SEEK_SET);
		fread(&block_offset,1,2,in);
		int pos=swapen(block_offset);
		//printf("%x\n",pos);
		int i=0;
		fseek(in,0L,SEEK_SET);
		if(size>10)
		{
			while(1)
			{	
			fread(&byte,1,1,in);
			if(byte==0xff)break;
			
			fread(&block_size,1,2,in);
			block_size=swapen(block_size);
			//printf("%x %x\n",byte,block_size);
			
			fread(&block_offset,1,2,in);
			block_offset=swapen(block_offset);
			//printf("%x\n",block_offset);
			fread(&buffer[block_offset-pos],block_size,1,in);
			}
			out=fopen("cart.rom","wb+");
			memcpy(&buffer[0x3f00],code,sizeof(code));
			memcpy(&buffer[0x3fe0],title,strlen(title));
			buffer[0x3fe0+strlen(title)]=0x04;
			buffer[0x3ffe]=0xef;
			fwrite(buffer,0x4000,1,out);
			fclose(out);
			printf("rom creata\n");
		} else
		printf("errore file\n");
		fclose(in);
	    
		return 0;
		
}