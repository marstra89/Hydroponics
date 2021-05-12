// Hydroponics

#include <mariadb/mysql.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#define MAX_TIME 85
#define MAX_TRIES 100
#define DHT11PIN 4 //7
#define relay 17
#define level 10
#define tmp_varning 7
#define level_varning 8
#define on_off 25
#define tmp_central 40.0	//temperatur i elcentral då varningsled tänds

// funktioner *****************************************************************************************

float getTemperature(int fd)
{
	int raw = wiringPiI2CReadReg16(fd, 0x00);
	raw = ((raw << 8) & 0xFF00) + (raw >> 8);
	return (float)((raw / 32.0) / 8.0);
}

int dht11_val[5]={0,0,0,0,0};

int dht11_read_val(int *h, int *t) {
    uint8_t lststate=HIGH;
    uint8_t counter=0;
    uint8_t j=0,i;

    for (i=0;i<5;i++) {
         dht11_val[i]=0;
    }

    pinMode(DHT11PIN,OUTPUT);
    digitalWrite(DHT11PIN,LOW);
    delay(18);
    digitalWrite(DHT11PIN,HIGH);
    delayMicroseconds(40);
    pinMode(DHT11PIN,INPUT);

    for (i=0;i<MAX_TIME;i++) {
        counter=0;
        while (digitalRead(DHT11PIN)==lststate){
            counter++;
            delayMicroseconds(1);
            if (counter==255)
                break;
        }
        lststate=digitalRead(DHT11PIN);
        if (counter==255)
             break;
        // top 3 transitions are ignored
        if ((i>=4) && (i%2==0)) {
            dht11_val[j/8]<<=1;
            if(counter>16)
                dht11_val[j/8]|=1;
            j++;
        }
    }

    // verify cheksum and print the verified data
    if ((j>=40) && (dht11_val[4]==((dht11_val[0]+dht11_val[1]+dht11_val[2]+dht11_val[3])& 0xFF))) {
        // Only return the integer part of humidity and temperature. The sensor
        // is not accurate enough for decimals anyway
        *h = dht11_val[0];
        *t = dht11_val[2];
        return 0;
    }
    else {
        printf(" invalid data\n");
        return 1;
    }
}

void finish_with_error(MYSQL *con) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
}


// slut funktioner*************************************************************************

int main(int argc, char *argv[]) {

   // pinMode(lägg alla pins här
    wiringPiSetupGpio();
    pinMode(relay, OUTPUT);
    pinMode(on_off, OUTPUT);
    pinMode(tmp_varning, OUTPUT);
    pinMode(level_varning, OUTPUT);
    pinMode(level, INPUT);

    digitalWrite(relay, LOW);
    digitalWrite(on_off, HIGH);
    digitalWrite(tmp_varning, LOW);
    digitalWrite(level_varning, LOW);

    // Definerar mysql handle
    MYSQL *con = mysql_init(NULL);
    
    if (con == NULL){
	fprintf(stderr, "1 %s\n", mysql_error(con));
	exit(1);
    }
    
    // Loggar in i databasen
    if (mysql_real_connect(con, "localhost", "logger", "raspberry", "sensor_data", 0, NULL, 0) == NULL){ // handle, host, user, pw, db, port, unix_socket, flags
	fprintf(stderr, "2 %s\n", mysql_error(con));
	mysql_close(con);
	exit(1);
    }



    //for(int look=0; look<3; look++){ //ändra till evighets-while
    int address = 0x4d;			//vår LM75A har address 4d i i2c
    int time_led_on= 7; 	//tid i timmar då led tänds
    int time_led_off= 23;	//tid i timmar då led släcks
    int h; //humidity
    int t; //temperature in degrees Celsius
    int h2o=1;
    float ph=14;

    //for(int i=0; i<3; i++){


    int print_time = 1;
    time_t current_time = time(NULL);
    struct tm tm = *localtime(&current_time);

    //for(int look=0; look<3; look++){ //ändra till evighets-while
	while(1){     //en evighet

		// read the sensor until we get a pair of valid measurements
		// but bail out if we tried too many times
		int retval=1;
		int tries=0;
		//for(int look=0; look<8; look++){
		while (retval != 0 && tries < MAX_TRIES)
		{
		    retval = dht11_read_val(&h, &t);
		    if (retval == 0) {
			if (print_time == 1) {
			    printf("%4d-%02d-%02d,", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
			    printf("%02d:%02d,", tm.tm_hour, tm.tm_min);
			}
			printf("%d procents luftfuktighet,", h);

			printf("%d Celsius\n", t);

		    } else {
			delay(1000);
		}
		    tries += 1;
		}


		// ********************************************************************************

		
		/* Read from I2C and print temperature */
		int fd = wiringPiI2CSetup(address);

		//for(int k=0; k<3; k++){

		    printf("%.2f Celsius just nu\n", getTemperature(fd) );
		if(getTemperature(fd)>tmp_central){
		digitalWrite(tmp_varning, HIGH);
		}
		else{
		digitalWrite(tmp_varning, LOW);
		}

		    if(digitalRead(level)<1){
			printf("vattennivå låg!\n");
			digitalWrite(level_varning, HIGH);
			h2o=0;
		    }
		    else{
			printf("Vattennivå ok\n");
			digitalWrite(level_varning, LOW);
			h2o=1;
		    }
		    delay(1000);
		//}

		// *********************************************************************************''

		// Create I2C bus
		int file;
		char *bus = "/dev/i2c-1";
		if ((file = open(bus, O_RDWR)) < 0)
		{
		    printf("Failed to open the bus. \n");
		    exit(1);
		}
		// Get I2C device, ADS7830 I2C address is 0x4b(72)
		ioctl(file, I2C_SLAVE, 0x4b);

		// Differential inputs, Channel-0, Channel-1 selected
		char config[1] = {0x04};
		write(file, config, 1);
		sleep(1);

		// Read 1 byte of data
		char data[1]={0};
		if(read(file, data, 1) != 1){
			printf("Error : Input/output Error \n");
		}
		else{
		    // Convert the data
		    int raw_adc = data[0];

		    //omvandlar enligt vår ph-sensors kalibrering
		    ph= (13.33-((1.0/15.0)*raw_adc));

		    // Output data to screen
		    printf("Ph-värde: %.2f \n", ph);
		    //printf("Raw-värde: %d \n", raw_adc);

		}

		/// START - SPARAR DATA FRÅN VARIABLER TILL DATABAS

		    char buf[1023] = {};
		    char query_string[] = {"INSERT INTO datalog(temp, humid, ph, h2o) VALUES(%d, %d, %f, %d)"}; 

		    sprintf(buf, query_string, t, h, ph, h2o);
		    if (mysql_query(con, buf)){
			finish_with_error(con);
		    }
		    ///// SLUT - SPARA DATA I DATABAS

	    
		    //RADERAR DATA SOM ÄR ÄLDRE ÄN 7 DAGAR
		    char buf2[1023] = {};
		    char query_string2[] = {"DELETE FROM datalog WHERE dateandtime < CURRENT_TIMESTAMP - INTERVAL 7 DAY"}; //RADERAR DATA SOM ÄR ÄLDRE ÄN 7 DAGAR

		    sprintf(buf2, query_string2);
		    if (mysql_query(con, buf2)){
			finish_with_error(con);
		    }
		    // SLUT

		    //styr lampa kolla seting 1 ggr per sek i 10 min
		
		    for(int wait=0; wait<600; wait++){
			     if(mysql_query(con, "SELECT led FROM datalog WHERE $
                                finish_with_error(con);
                                }
                            MYSQL_RES *result = mysql_store_result(con);
                            if (result == NULL){
                                finish_with_error(con);
                                }
                            int num_fields = mysql_num_fields(result);
                            MYSQL_ROW row;
                            while ((row = mysql_fetch_row(result))){
                                for(int d =0; d< num_fields; d++){
                                        //lampa= strcmp(row[d], row[d-1]);

                                        lampa= atoi(row[d]);
                                        printf("string: %s ", row[d] ? row[d] :$
                                        printf("lampa: %d", lampa);
                                }
                                printf("\n");
                            }
                            mysql_free_result(result);



                           if(lampa==1){  // if(tm.tm_hour> time_led_on && tm.tm_hour< time_led_off){
				    digitalWrite(relay, HIGH);
				    delay(1000);
				}
				else{
				    digitalWrite(relay, LOW);
				    delay(1000);
				}
		    }
		    

    } //slut på en evighet
    digitalWrite(relay, LOW);
    digitalWrite(level_varning, LOW);
    digitalWrite(on_off, LOW);
    digitalWrite(tmp_varning, LOW);
    return 0;
    //****************************************************************************



}
