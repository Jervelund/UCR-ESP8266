#define UCR_DEBUG 1
#include <Arduino.h>
#include <UCR.h>
#include <Servo.h>

#ifndef STASSID
#define STASSID "TODO"
#define STAPSK "TODO"
#endif

#define LED_BUILTIN 2
#define AXIS_MAX 32767
#define AXIS_MIN -32768

const char *ssid = STASSID;
const char *password = STAPSK;
unsigned int localPort = 8080;

// Other vars
#define DEADMAN_TIMEOUT 500
#define ACCELERATOR_PIN 15
#define STEERING_PIN 5
#define ACCELERATOR 0
#define STEERING 1
UCR ucr(ssid, password, localPort);
Servo steering;
Servo accelerator;

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\nsetup()");

  pinMode(LED_BUILTIN, OUTPUT);

  ucr.setName("UCR-Traxxas");
  ucr.addOutputButton("LED", 0);
  ucr.addOutputAxis("Acc", ACCELERATOR);
  ucr.addOutputAxis("Steer", STEERING);
  ucr.addOutputEvent("Horn", 0);

  ucr.addInputButton("X", 0);
  ucr.addInputButton("Y", 1);
  ucr.addInputButton("A", 2);
  ucr.addInputButton("B", 3);
  ucr.addInputAxis("Axis X", 0);
  ucr.addInputAxis("Axis Y", 1);
  ucr.setUpdateRate(10000);
  ucr.setTimeout(30000);

  ucr.begin();

  /*
  // Manually set subscriber address
  IPAddress addr;
  addr.fromString("192.168.1.243");
  ucr.setSubscriber(addr,8090);
  */

  // Arm Traxxas ESC
  accelerator.attach(ACCELERATOR_PIN);
  accelerator.write(91);
  delay(1000);

  steering.attach(STEERING_PIN);
  steering.write(90);
}

unsigned long last_change = 0;
int last_state = 0;

void loop()
{
  if (millis() - last_change >= 2000){
    last_change = millis();
    switch(last_state++){
      case 0:
        ucr.writeButton(0, 1);
      break;
      case 1:
        ucr.writeButton(0, 0);
      break;
      case 2:
        ucr.writeButton(1, 1);
      break;
      case 3:
        ucr.writeButton(1, 0);
      break;
      case 4:
        ucr.writeButton(2, 1);
      break;
      case 5:
        ucr.writeButton(2, 0);
      break;
      case 6:
        ucr.writeButton(3, 1);
      break;
      case 7:
        ucr.writeButton(3, 0);
      break;
      default:
        last_state = 0;
      break;
    }
  }

  ucr.update();

  if (millis() - ucr.lastUpdateMillis() <= DEADMAN_TIMEOUT)
  {
    analogWrite(LED_BUILTIN, map(abs((long)ucr.readAxis(0)), 0,AXIS_MAX, 1024, 0));
    accelerator.write(map(ucr.readAxis(ACCELERATOR),AXIS_MIN,AXIS_MAX,0,180));
    steering.write(map(ucr.readAxis(STEERING),AXIS_MIN,AXIS_MAX,35,145));

    if (ucr.readEvent(0)) {
      // TODO
    }
  }
  else
  {
    Serial.println("Connection lost");
  }
}