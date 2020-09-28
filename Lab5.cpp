#include<iostream>
#include<unistd.h>
#include<wiringPi.h>
#include<iomanip>
#include<iomanip>
using namespace std;

#define USING_DHT11      true
#define DHT_GPIO         22      // Using GPIO 22 for this example
#define LH_THRESHOLD     26      // Low=~14, High=~38 - pick avg.
#define Room_Temp_Threshold 31

#define PWM_SERVO     18      // this is PWM0, pin 12
#define BUTTON_GPIO   27      // this is GPIO27, pin 13
#define LEFT          29      // manually calibrated values
#define RIGHT         118     // for the left, right and
#define CENTER        73      // center servo positions
bool sweeping = true;         // sweep servo until button pressed

int getTemperature(){
   int humid = 0, temp = 0;
   cout << "Starting the one-wire sensor program" << endl;
   wiringPiSetupGpio();
   piHiPri(99);

TRYAGAIN:                        // If checksum fails (come back here)
   unsigned char data[5] = {0,0,0,0,0};
   pinMode(DHT_GPIO, OUTPUT);                 // gpio starts as output
   digitalWrite(DHT_GPIO, LOW);               // pull the line low
   usleep(18000);                             // wait for 18ms
   digitalWrite(DHT_GPIO, HIGH);              // set the line high
   pinMode(DHT_GPIO, INPUT);                  // now gpio is an input

   // ignore 1st, 2nd pulse
   do { delayMicroseconds(1); } while(digitalRead(DHT_GPIO)==HIGH);
   do { delayMicroseconds(1); } while(digitalRead(DHT_GPIO)==LOW);
   do { delayMicroseconds(1); } while(digitalRead(DHT_GPIO)==HIGH);

   for(int d=0; d<5; d++) {       // for each data byte
      // read 8 bits
      for(int i=0; i<8; i++) {    // for each bit of data
         do { delayMicroseconds(1); } while(digitalRead(DHT_GPIO)==LOW);
         int width = 0;           // measure width of each high
         do {
            width++;
            delayMicroseconds(1);
            if(width>1000) break; // missed a pulse -- data invalid!
         } while(digitalRead(DHT_GPIO)==HIGH);    // time it!
         // shift in the data, msb first if width > the threshold
         data[d] = data[d] | ((width > LH_THRESHOLD) << (7-i));
      }
   }

   humid = data[0] * 10;            // one byte - no fractional part
   temp = data[2] * 10;             // multiplying to keep code concise

   unsigned char chk = 0;   // checksum will overflow automatically
   for(int i=0; i<4; i++){ chk+= data[i]; }
   if(chk==data[4]){
      cout << "The checksum is good" << endl;
      cout << "The temperature is " << (float)temp/10 << "°C" << endl;
      cout << "The humidity is " << (float)humid/10 << "%" << endl;
   }
   else {
      cout << "Checksum bad - data error - trying again!" << endl;
      usleep(2000000);   // delay for 1-2 seconds between readings
      goto TRYAGAIN;
   }
   return ((float)temp/10);
}

void buttonPress(void) {      // ISR on button press - not debounced
   cout << "Button was pressed -- finishing sweep." << endl;
   sweeping = false;          // the while() loop should end soon
   int checkWhereMotorShouldGo = getTemperature();

   if (checkWhereMotorShouldGo >= Room_Temp_Threshold){
     pwmWrite(PWM_SERVO, RIGHT);
   } else {
     pwmWrite(PWM_SERVO, LEFT);
   }
 }

int main() {                             // must be run as root
   wiringPiSetupGpio();                  // use the GPIO numbering
   pinMode(PWM_SERVO, PWM_OUTPUT);       // the PWM servo
   pinMode(BUTTON_GPIO, INPUT);          // the button input
   wiringPiISR(BUTTON_GPIO, INT_EDGE_RISING, &buttonPress);
   pwmSetMode(PWM_MODE_MS);              // use a fixed frequency
   pwmSetRange(1000);                    // 1000 steps
   pwmSetClock(384);                     // gives 50Hz precisely

   while(sweeping) {
      for(int i=0; i<100000; i++) {       // infinite loop
         usleep(10000);
       }
   }
   return 0;
}
