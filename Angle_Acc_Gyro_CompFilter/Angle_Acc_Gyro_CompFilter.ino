// 《 상보필터를 적용한 Roll과 Pitch, Yaw의 각도 구하기 실습  》         
/* 아래 코드관련 실습에 대한 설명과 회로도 및 자료는 https://rasino.tistory.com/ 에 있습니다    */

#include<Wire.h>

const int MPU_ADDR = 0x68;    // I2C통신을 위한 MPU6050의 주소
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;   // 가속도(Acceleration)와 자이로(Gyro)
double angleAcX, angleAcY, angleAcZ;
double angleGyX, angleGyY, angleGyZ;
double angleFiX, angleFiY, angleFiZ;

const double RADIAN_TO_DEGREE = 180 / 3.14159;  
const double DEG_PER_SEC = 32767 / 250;    // 1초에 회전하는 각도: 각속도를 각도로 변환하는 비율
const double ALPHA = 1 / (1 + 0.04); //가중치값: T/(T+dt) -> 1(T)cycle 도는 동안의 loop Time(dt=0.04)
// GyX, GyY, GyZ 값의 범위 : -32768 ~ +32767 (16비트 정수범위)


unsigned long now = 0;   // 현재 시간 저장용 변수
unsigned long past = 0;  // 이전 시간 저장용 변수
double dt = 0;           // 한 사이클 동안 걸린 시간 변수 

double averAcX, averAcY, averAcZ;
double averGyX, averGyY, averGyZ;

void setup() {
  initSensor();
  Serial.begin(115200); //baud rate 지정
  caliSensor();   //  초기 센서 캘리브레이션 함수 호출
  past = millis(); // past에 현재 시간 저장  
}

void loop() {
  
  getData(); 
  getDT();

  //Accelerometer: angle 계산
  angleAcX = atan(AcY / sqrt(pow(AcX, 2) + pow(AcZ, 2)));//삼각함수 사용해 roll 각도 계산
  angleAcX *= RADIAN_TO_DEGREE;
  angleAcY = atan(-AcX / sqrt(pow(AcY, 2) + pow(AcZ, 2)));//삼각함수 사용해 pitch 각도 계산
  angleAcY *= RADIAN_TO_DEGREE;
  // angleAcZ 값: 가속도 센서만 이용해서는 원하는 값 얻기 어려움
  
  // 가속도 현재 값에서 초기평균값을 빼서 센서값에 대한 보정
  //시간에 따른 각도의 변화 계산 및 누적
  angleGyX += ((GyX - averGyX) / DEG_PER_SEC) * dt; // ((current 각속도 - initial 각속도)/DEG_PER_SEC)*시간 간격 = ((보정된 current 각속도)/DEG_PER_SEC)*시간 간격 = (current 각도 변화율)*시간 간격 = current 각도 변화량 
  angleGyY += ((GyY - averGyY) / DEG_PER_SEC) * dt;
  angleGyZ += ((GyZ - averGyZ) / DEG_PER_SEC) * dt;

  // 상보필터 처리를 위한 임시각도 저장
  double angleTmpX = angleFiX + angleGyX * dt;//current angle = inital_angle + angleGyx*dt
  double angleTmpY = angleFiY + angleGyY * dt;
  double angleTmpZ = angleFiZ + angleGyZ * dt;

  // (상보필터 값 처리) 임시 각도에 0.96가속도 센서로 얻어진 각도 0.04의 비중을 두어 현재 각도를 구함.
  angleFiX = ALPHA * angleTmpX + (1.0 - ALPHA) * angleAcX; // F=(1-alpha)*Gyro + alpha *Acc
  angleFiY = ALPHA * angleTmpY + (1.0 - ALPHA) * angleAcY;
  angleFiZ = angleGyZ;    // Z축은 자이로 센서만을 이용하여 구함.
  
  Serial.print("AngleAcX:");
  Serial.print(angleAcX);
  Serial.print("  FilteredX:");
  Serial.print(angleFiX);
  Serial.print("\t AngleAcY:");
  Serial.print(angleAcY);
  Serial.print("\t FilteredY:");
  Serial.print(angleFiY);
  Serial.print("\t AngleGyZ:");
  Serial.print(angleGyZ);
  Serial.print("\t FilteredZ:");
  Serial.println(angleFiZ);

//  Serial.print("Angle Gyro X:");
//  Serial.print(angleGyX);
//  Serial.print("\t\t Angle Gyro y:");
//  Serial.print(angleGyY);  
//  Serial.print("\t\t Angle Gyro Z:");
//  Serial.println(angleGyZ);  
//  delay(20);
}

void initSensor() {
  Wire.begin();//I2C 통신 초기화
  Wire.beginTransmission(MPU_ADDR);   // master에서 전송을 시작하기 위해 slave의 주소값 지정
  Wire.write(0x6B);    // 데이터 버퍼에 실제로 전송될 데이터 저장하는 함수
  Wire.write(0);//
  Wire.endTransmission(true);//데이터 버퍼에 저장된 데이터 전송
}

void getData() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);   // AcX 레지스터 위치(주소)를 지칭합니다
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);  // master 모드에서 slave로  14byte의 데이터를 요청
  AcX = Wire.read() << 8 | Wire.read(); //두 개의 나뉘어진 바이트를 하나로 이어 붙여서 각 변수에 저장
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Tmp = Wire.read() << 8 | Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();
}

// loop 한 사이클동안 걸리는 시간을 알기위한 함수
void getDT() {
  now = millis();   
  dt = (now - past) / 1000.0;  
  past = now;
}

// 센서의 초기값을 10회 정도 평균값으로 구하여 저장하는 함수
void caliSensor() {
  double sumAcX = 0 , sumAcY = 0, sumAcZ = 0;
  double sumGyX = 0 , sumGyY = 0, sumGyZ = 0;
  getData();
  for (int i=0;i<10;i++) {
    getData();
    sumAcX+=AcX;  sumAcY+=AcY;  sumAcZ+=AcZ;
    sumGyX+=GyX;  sumGyY+=GyY;  sumGyZ+=GyZ;
  }
  averAcX=sumAcX/10;  averAcY=sumAcY/10;  averAcZ=sumAcY/10; //acc 평균값 계산
  averGyX=sumGyX/10;  averGyY=sumGyY/10;  averGyZ=sumGyZ/10; //gyro 평균값 계산
}