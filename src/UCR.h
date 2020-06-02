/*
  UCR ESP8266 lib
*/

#include <WiFiUdp.h>

#define UCR_IN_BUTTON_COUNT 64
#define UCR_IN_AXIS_COUNT 32
#define UCR_IN_DELTA_COUNT 32
#define UCR_IN_EVENT_COUNT 32

#define UCR_OUT_BUTTON_COUNT 64
#define UCR_OUT_AXIS_COUNT 32
#define UCR_OUT_DELTA_COUNT 32
#define UCR_OUT_EVENT_COUNT 32

#define UCR_ANTIFLOOD_MS 5
#define UCR_KEEPALIVE_MS 10000
#define UCR_SUBSCRIBER_TIMEOUT_MS 300000

class UCR
{

public:
  UCR(const char *ssid, const char *password);
  UCR(const char *ssid, const char *password, uint16_t port);
  void begin();
  bool update();
  void resetValues();
  void setName(const char *name);
  void setUpdateRate(uint16_t miliseconds);
  bool updateSubscriber();
  void setSubscriber(IPAddress address, uint16_t port);
  void setTimeout(unsigned long timeout);
  bool sendBindResponse(int index, char *type, short value);

  void addInputButton(const char *name, int index);
  void addInputAxis(const char *name, int index);
  void addInputDelta(const char *name, int index);
  void addInputEvent(const char *name, int index);

  void writeButton(int index, bool value);
  void writeAxis(int index, short value);
  void writeDelta(int index, short value);
  void writeEvent(int index, bool value);

  void addOutputButton(const char *name, int index);
  void addOutputAxis(const char *name, int index);
  void addOutputDelta(const char *name, int index);
  void addOutputEvent(const char *name, int index);

  bool readButton(int index);
  short readAxis(int index);
  short readDelta(int index);
  bool readEvent(int index);

  unsigned long lastUpdateMillis();
  unsigned long lastReceiveMillis();
  unsigned long lastSendMillis();

private:
  void setupWiFi();
  void setupMDNS();
  bool receiveUdp();

  WiFiUDP Udp;
  char _hostString[32] = {0};
  unsigned int _localPort = 8080;
  char _name[32] = {0};
  const char *_ssid;
  const char *_password;
  uint16_t _updateRate = 20;
  uint16_t _port;
  unsigned long _timeout = UCR_SUBSCRIBER_TIMEOUT_MS;
  unsigned long _lastUpdateMillis = 0;
  unsigned long _lastReceiveMillis = 0;
  unsigned long _lastSendMillis = 0;
  unsigned int _sequenceNo = 0;
  bool _inputDirty = 0;
  IPAddress _remoteIP;
  uint16_t _remotePort = 0;
  bool _bindMode = false;
  bool _subscribed = false;

  const char *_inButtonList[UCR_IN_BUTTON_COUNT] = {0};
  const char *_inAxisList[UCR_IN_AXIS_COUNT] = {0};
  const char *_inDeltaList[UCR_IN_DELTA_COUNT] = {0};
  const char *_inEventList[UCR_IN_EVENT_COUNT] = {0};

  bool inButtonData[UCR_IN_BUTTON_COUNT] = {0};
  short inAxisData[UCR_IN_AXIS_COUNT] = {0};
  short inDeltaData[UCR_IN_DELTA_COUNT] = {0};
  bool inEventData[UCR_IN_EVENT_COUNT] = {0};

  const char *_outButtonList[UCR_OUT_BUTTON_COUNT] = {0};
  const char *_outAxisList[UCR_OUT_AXIS_COUNT] = {0};
  const char *_outDeltaList[UCR_OUT_DELTA_COUNT] = {0};
  const char *_outEventList[UCR_OUT_EVENT_COUNT] = {0};

  bool outButtonData[UCR_OUT_BUTTON_COUNT] = {0};
  short outAxisData[UCR_OUT_AXIS_COUNT] = {0};
  short outDeltaData[UCR_OUT_DELTA_COUNT] = {0};
  bool outEventData[UCR_OUT_EVENT_COUNT] = {0};

  char incomingPacketBuffer[255];
};
