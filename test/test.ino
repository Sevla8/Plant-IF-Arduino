void setup() {

  Serial.begin(9600);
  while (!Serial) {

    ; // wait for serial port to connect. Needed for native USB port only

  }
  // put your setup code here, to run once:
  Serial.println("Hello world");
}

void loop() {
  // put your main code here, to run repeatedly:
  
  Serial.println("Sensor Data");
  Serial.println(analogRead(A0));
  Serial.println(analogRead(10));
  Serial.println(analogRead(11));
  Serial.println(analogRead(12));
  Serial.println(analogRead(13));
  delay(1000);
}
