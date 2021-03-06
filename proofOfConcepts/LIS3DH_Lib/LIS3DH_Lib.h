//#include <Wire.h>

//**********************************************************************************************//
//					DEFINES						 	//
//**********************************************************************************************//

/* 2 Measurement of acceleration values using motion sensor LIS3DH 3 */ 
#define ADDRESS_LIS3DH 0x18
#define CTRL_REG1 0x20 
#define CTRL_REG4 0x23 
#define CTRL_REG5 0x24 
#define STATUS_REG 0x27 
#define OUT_X_L 0x28 

//**********************************************************************************************//
//					GLOBAL VARIABLES				 	//
//**********************************************************************************************//

byte buffer[ 6 ]; 
byte statusReg; 
boolean ready = false ; 
int outX, outY, outZ; 
int xVal, yVal, zVal; 

//**********************************************************************************************//
//				   FUNCTIONS DECLARATIONS				 	//
//**********************************************************************************************//

void accel_init();
int getData();   //updates xVal, yVal, zVal
int getAngle();  //uses getData() to return the angle

//**********************************************************************************************//
//					FUNCTIONS					 	//
//**********************************************************************************************//

void accel_init(){
  Wire.begin();
  delay( 5 ); // 5 ms boot procedure 26 27 // reboot memory content, to make a clean start 28 
  Wire.beginTransmission(ADDRESS_LIS3DH); 
  Wire.write(CTRL_REG5); 
  Wire.write( 0x80 ); 
  Wire.endTransmission(); 
  delay( 5 ); // set ODR = 1 Hz, normal mode, x/y/z axis enabled
  Wire.beginTransmission(ADDRESS_LIS3DH); 
  Wire.write(CTRL_REG1); 
  Wire.write( 0x27 ); // 17 = 1, 27 = 10, 47 = 50 Hz
  Wire.endTransmission(); // set BDU= 1, scale = +/-2g, high resolution enabled 42 
  Wire.beginTransmission(ADDRESS_LIS3DH); 
  Wire.write(CTRL_REG4);
  Wire.write( 0x80 ); 
  Wire.endTransmission(); 
}

int getData(){
 // read STATUS_REG 
  while (ready == false ){
    Wire.beginTransmission(ADDRESS_LIS3DH); 
    Wire.write(STATUS_REG); 
    Wire.endTransmission(); 
    Wire.requestFrom(ADDRESS_LIS3DH, 1 ); 
    if (Wire.available() >= 1 ){ 
      statusReg = Wire.read(); 
    }
    if (bitRead(statusReg, 3 ) == 1 ){  // new data available 
      ready = true ; 
    }
    delay( 2 );
  }
    
  if (bitRead(statusReg, 7 ) == 1 ){
     Serial.println( " Some data have been overwritten. " ); 
  }
  // read the result 
  Wire.beginTransmission(ADDRESS_LIS3DH); 
  Wire.write(OUT_X_L | 0x80 ); // read multiple bytes 
  Wire.endTransmission(); 
  Wire.requestFrom(ADDRESS_LIS3DH, 6 );
  if (Wire.available() >= 6 ){
    for ( int i = 0 ; i < 6 ; i++ ){
      buffer[i] = Wire.read(); 
    }
  }
  // calculation 
  outX = (buffer[ 1 ] << 8 ) | buffer[ 0 ]; 
  outY = (buffer[ 3 ] << 8 ) | buffer[ 2 ]; 
  outZ = (buffer[ 5 ] << 8 ) | buffer[ 4 ]; 
  xVal = outX / 16 ; 
  yVal = outY / 16 ; 
  zVal = outZ / 16 ; 
  
  /*
  
  Serial.print( " outX: " ); 
  Serial.print(xVal); 
  Serial.print( " " ); 
  Serial.print( " outY: " ); 
  Serial.print(yVal); 
  Serial.print( " " );
  Serial.print( " outZ: " ); 
  Serial.println(zVal); 
  
  */
  
  ready = false ; 
  
  // return [xVal, yVal, zVal]
}

int getAngle(){
  getData();
  int curAngle;
  curAngle=(180*atan(((float)xVal)/yVal)/PI)+90;
  if(yVal>0) curAngle=180+curAngle;
  
  return curAngle;
}
