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

uint8_t ucr_output_led;
uint8_t ucr_output_accelerator;
uint8_t ucr_output_steering;
uint8_t ucr_output_horn;

uint8_t ucr_input_button_x;
uint8_t ucr_input_button_y;
uint8_t ucr_input_button_a;
uint8_t ucr_input_button_b;
uint8_t ucr_input_axis_x;
uint8_t ucr_input_axis_y;

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\nsetup()");

  pinMode(LED_BUILTIN, OUTPUT);

  ucr.setName("UCR-Traxxas");
  ucr_output_led = ucr.addOutputButton("LED");
  ucr_output_accelerator = ucr.addOutputAxis("Acc");
  ucr_output_steering = ucr.addOutputAxis("Steer");
  ucr_output_horn = ucr.addOutputEvent("Horn");

  ucr_input_button_x = ucr.addInputButton("X");
  ucr_input_button_y = ucr.addInputButton("Y");
  ucr_input_button_a = ucr.addInputButton("A");
  ucr_input_button_b = ucr.addInputButton("B");
  ucr_input_axis_x = ucr.addInputAxis("Axis X");
  ucr_input_axis_y = ucr.addInputAxis("Axis Y");
  ucr.setUpdateRate(10000);
  ucr.setTimeout(30000);

  ucr.begin();

  IPAddress addr;
  addr.fromString("192.168.1.243");
  //ucr.setSubscriber(addr,8090);

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
    ucr.writeAxis(ucr_input_button_x, millis());
    switch(last_state++){
      case 0:
        ucr.writeButton(ucr_input_button_x, 1);
      break;
      case 1:
        ucr.writeButton(ucr_input_button_x, 0);
      break;
      case 2:
        ucr.writeButton(ucr_input_button_y, 1);
      break;
      case 3:
        ucr.writeButton(ucr_input_button_y, 0);
      break;
      case 4:
        ucr.writeButton(ucr_input_button_a, 1);
      break;
      case 5:
        ucr.writeButton(ucr_input_button_a, 0);
      break;
      case 6:
        ucr.writeButton(ucr_input_button_b, 1);
      break;
      case 7:
        ucr.writeButton(ucr_input_button_b, 0);
      break;
      default:
        last_state = 0;
      break;
    }
  }

  ucr.update();

  if (ucr.connectionAlive())
  {
    analogWrite(LED_BUILTIN, map(abs((long)ucr.readAxis(ucr_output_led)), 0,AXIS_MAX, 1024, 0));
    accelerator.write(map(ucr.readAxis(ucr_output_accelerator),AXIS_MIN,AXIS_MAX,0,180));
    steering.write(map(ucr.readAxis(ucr_output_steering),AXIS_MIN,AXIS_MAX,35,145));

    if (ucr.readEvent(ucr_output_horn)) {
      // TODO
    }
  }
  else
  {
    Serial.println("Connection lost");
    delay(500);
  }
}
