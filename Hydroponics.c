
#include <stdio.h>
#include <wiringPiI2C.h>   //LM75A


float getTemperature(int fd); //LM75A

int main (int argc, char *argv[])
{

//************************************LM75A*********************************************************
    int address = 0x4d;     //våran LM75A har addressen 4d i i2c
    if (1 < argc){
		address = (int)strtol(argv[1], NULL, 0);
	}
    int fd = wiringPiI2CSetup(address);



    While(1){
        for(int k=10; k<0; k--){

	        printf("%.2f Celsius just nu\n", getTemperature(fd) ); //temp från LM75A


            printf("hello darkness my old friend"); //test
        }



    }
  
 
 



	
	
  
  
  
  


  return 0;
  
  float getTemperature(int fd){
	  int raw = wiringPiI2CReadReg16(fd, 0x00);
	  raw = ((raw << 8) & 0xFF00) + (raw >> 8);
	  return (float)((raw / 32.0) / 8.0);
    }
}
