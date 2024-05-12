int cpin;
int top=12;
int low=13;
int mid=14;




void setup() {
  Serial.begin(9600);
  pinMode(top,INPUT_PULLUP);
  pinMode(low,INPUT_PULLUP);
  pinMode(mid,INPUT_PULLUP);
  Serial.println("START");

}

void loop() {

Serial.print( digitalRead(low));
Serial.print(digitalRead(mid));
Serial.println(digitalRead(top)); 
int t=digitalRead(top);
int l=digitalRead(low);
int m=digitalRead(mid);
if(l==0 && m==1 && t==1){
  Serial.println("Water level low");
  delay(500);
}

else if(l==0 && m==0 && t==1){

  Serial.println("Water level mid");
  delay(500);
}
 

else if(l==0 && m==0 && t==0){

  Serial.println("Water level top");
  delay(500);
}
else{
Serial.println("tank empty");
  
}

}
