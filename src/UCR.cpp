#include "UCR.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#ifdef DEBUG
#define D(x) (x)
#else 
#define D(x) do{}while(0)
#endif

#define D(x) (x)

#define IN_BUTTONS "b"
#define IN_AXES "a"
#define IN_DELTAS "d"
#define IN_EVENTS "e"

#define OUT_BUTTONS "b"
#define OUT_AXES "a"
#define OUT_DELTAS "d"
#define OUT_EVENTS "e"

#define MSG_HEARTBEAT_REQUEST 0
#define MSG_HEARTBEAT_RESPONSE 1
#define MSG_DESCRIPTOR_LIST_REQUEST 2
#define MSG_DESCRIPTOR_LIST_RESPONSE 3
#define MSG_SET_OUTPUTS 4
#define MSG_SUBSCRIBE_TO_INPUTS 5
#define MSG_UNSUBSCRIBE_FROM_INPUTS 6
#define MSG_UPDATE_SUBSCRIBER 7
#define MSG_BIND_START 8
#define MSG_BIND_STOP 9
#define MSG_BIND_RESPONSE 10

UCR::UCR(const char *ssid, const char *password)
{
    UCR(ssid, password, 8080);
}

UCR::UCR(const char *ssid, const char *password, uint16_t port)
{
    _ssid = ssid;
    _password = password;
    _port = port;
}

void UCR::setTimeout(unsigned long timeout)
{
    _timeout = timeout;
}

unsigned long UCR::lastUpdateMillis()
{
    return _lastUpdateMillis;
}

unsigned long UCR::lastReceiveMillis()
{
    return _lastReceiveMillis;
}

unsigned long UCR::lastSendMillis()
{
    return _lastSendMillis;
}

void UCR::begin()
{
    Serial.println("UCR begin");
    setupWiFi();
    setupMDNS();
    Udp.begin(_port);
}

void UCR::setupWiFi() 
{
    D(Serial.println("Setup WiFi"));

    if (strlen(_name) == 0) 
    {
        sprintf(_hostString, "UCR_%06X", ESP.getChipId());
    }
    else
    {
        strncpy(_hostString, _name, (sizeof(_hostString) - 1));
    }

    D(Serial.print("Hostname: "));
    D(Serial.println(_hostString));

    WiFi.hostname(_hostString);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(250);
        D(Serial.print("."));
    }

    Serial.println();
    Serial.print("Connected to ");
    Serial.println(_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void UCR::setupMDNS()
{
    Serial.println("Setup MDNS");
    if (!MDNS.begin(_hostString))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
    MDNS.addService("ucr", "udp", _localPort);
}

void UCR::setName(const char *name)
{
    strncpy(_name, name, (sizeof(_name) - 1));
}

void UCR::setUpdateRate(uint16_t miliseconds) {
    _updateRate = miliseconds;
}

void UCR::addInputButton(const char *name, int index)
{
    if (index >= UCR_IN_BUTTON_COUNT) return;
    _inButtonList[index] = name;
}

void UCR::addInputAxis(const char *name, int index)
{
    if (index >= UCR_IN_AXIS_COUNT) return;
    _inAxisList[index] = name;
}

void UCR::addInputDelta(const char *name, int index)
{
    if (index >= UCR_IN_DELTA_COUNT) return;
    _inDeltaList[index] = name;
}

void UCR::addInputEvent(const char *name, int index)
{
    if (index >= UCR_IN_EVENT_COUNT) return;
    _inEventList[index] = name;
}

void UCR::writeButton(int index, bool value){
    if (index >= UCR_IN_BUTTON_COUNT) return;
    if (inButtonData[index] == value) return;
    inButtonData[index] = value;
    _inputDirty = 1;
    if(_bindMode) sendBindResponse(index, "b", value);
}

void UCR::writeAxis(int index, short value){
    if (index >= UCR_IN_AXIS_COUNT) return;
    if (inAxisData[index] == value) return;
    inAxisData[index] = value;
    _inputDirty = 1;
    if(_bindMode) sendBindResponse(index, "a", value);
}

void UCR::writeDelta(int index, short value){
    if (index >= UCR_IN_DELTA_COUNT) return;
    if (inDeltaData[index] == value) return;
    inDeltaData[index] = value;
    _inputDirty = 1;
    if(_bindMode) sendBindResponse(index, "d", value);
}

void UCR::writeEvent(int index, bool value){
    // IDEA: Add option to use event as flag (that should be cleared, this allows us to call multiple .update()s/receive new packets without missing an event). This won't help in case of dropped UDP packets, as we'll never know about the event.
    if (index >= UCR_IN_EVENT_COUNT) return;
    if (inEventData[index] == value) return;
    inEventData[index] = value;
    _inputDirty = 1;
    if(_bindMode) sendBindResponse(index, "e", value);

}

void UCR::addOutputButton(const char *name, int index)
{
    if (index >= UCR_OUT_BUTTON_COUNT) return;
    _outButtonList[index] = name;
}

void UCR::addOutputAxis(const char *name, int index)
{
    if (index >= UCR_OUT_AXIS_COUNT) return;
    _outAxisList[index] = name;
}

void UCR::addOutputDelta(const char *name, int index)
{
    if (index >= UCR_OUT_DELTA_COUNT) return;
    _outDeltaList[index] = name;
}

void UCR::addOutputEvent(const char *name, int index)
{
    if (index >= UCR_OUT_EVENT_COUNT) return;
    _outEventList[index] = name;
}

bool UCR::readButton(int index){
    if (index >= UCR_OUT_BUTTON_COUNT) return 0;
    return outButtonData[index];
}

short UCR::readAxis(int index){
    if (index >= UCR_OUT_AXIS_COUNT) return 0;
    return outAxisData[index];
}

short UCR::readDelta(int index){
    if (index >= UCR_OUT_DELTA_COUNT) return 0;
    return outDeltaData[index];
}

bool UCR::readEvent(int index){
    // IDEA: Add option to use event as flag (that should be cleared, this allows us to call multiple .update()s/receive new packets without missing an event). This won't help in case of dropped UDP packets, as we'll never know about the event.
    if (index >= UCR_OUT_EVENT_COUNT) return 0;
    return outEventData[index];
}

bool UCR::update()
{
    _lastUpdateMillis = millis();

    // Update mDNS
    MDNS.update();

    // Process received UDP packets
    receiveUdp();

    // Check if we are have a subscriber, and it's time to send an update
    if(_subscribed){
        unsigned long delta = _lastUpdateMillis - _lastSendMillis;
        // If we have crossed the limit of transmittion rate
        // If input is dirty and more than 10 ms since last packet was sent
        if(delta >= _updateRate || (_inputDirty && delta >= UCR_ANTIFLOOD_MS)){
            updateSubscriber();
        }

        // Check if we haven't heard from remote host in a while, and should give up sending
        if(_lastUpdateMillis - _lastReceiveMillis >= _timeout){
            D(Serial.println("Time since last packet from remote host has exceeded subscriber timeout treshold: Unsubscribing remote host."));
            _subscribed = false;
        }
    }

    return true;
}

void UCR::resetValues(){
    memset(outButtonData, 0, sizeof(outButtonData));
    memset(outAxisData, 0, sizeof(outAxisData));
    memset(outDeltaData, 0, sizeof(outDeltaData));
    memset(outEventData, 0, sizeof(outEventData));
}

void addDescriptorList(JsonObject *doc, const char *name, const char **list, int size)
{
    JsonArray descriptor_array = doc->createNestedArray(name);
    for (int i = 0; i < size; i++)
    {
        if (list[i] != nullptr)
        {
            JsonObject descriptor = descriptor_array.createNestedObject();
            descriptor["k"] = list[i];
            descriptor["v"] = i;
        }
    }
}

bool UCR::receiveUdp()
{
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {

        int len = Udp.read(incomingPacketBuffer, sizeof(incomingPacketBuffer));
        if (len > 0)
        {
            incomingPacketBuffer[len] = 0;
        }
        // D(Serial.printf("UDP packet contents: %s\n", incomingPacketBuffer));

        // Parse packet
        StaticJsonDocument<500> request;
        DeserializationError error = deserializeMsgPack(request, incomingPacketBuffer);

        if (error)
        {
            D(Serial.print("deserializeMsgPack() failed: "));
            D(Serial.println(error.c_str()));
            return false;
        }
        _lastReceiveMillis = _lastUpdateMillis;

        int msgType = request["MsgType"];

        D(Serial.print("RX: "));
        D(serializeJson(request, Serial));
        D(Serial.println());

        StaticJsonDocument<1000> response;
        response["hostname"] = _hostString;

        if (msgType == MSG_DESCRIPTOR_LIST_REQUEST)
        {
            // Let IOWrapper know what rate we prefer (if IOWrapper sends too often, our receive buffer will overflow)
            response["rate"] = _updateRate;
            response["MsgType"] = MSG_DESCRIPTOR_LIST_RESPONSE;

            // Add inputs
            JsonObject response_in = response.createNestedObject("i");
            addDescriptorList(&response_in, IN_BUTTONS, _inButtonList, UCR_IN_BUTTON_COUNT);
            addDescriptorList(&response_in, IN_AXES, _inAxisList, UCR_IN_AXIS_COUNT);
            addDescriptorList(&response_in, IN_DELTAS, _inDeltaList, UCR_IN_DELTA_COUNT);
            addDescriptorList(&response_in, IN_EVENTS, _inEventList, UCR_IN_EVENT_COUNT);

            // Add outputs
            JsonObject response_out = response.createNestedObject("o");
            addDescriptorList(&response_out, OUT_BUTTONS, _outButtonList, UCR_OUT_BUTTON_COUNT);
            addDescriptorList(&response_out, OUT_AXES, _outAxisList, UCR_OUT_AXIS_COUNT);
            addDescriptorList(&response_out, OUT_DELTAS, _outDeltaList, UCR_OUT_DELTA_COUNT);
            addDescriptorList(&response_out, OUT_EVENTS, _outEventList, UCR_OUT_EVENT_COUNT);
        }

        if (msgType == MSG_SET_OUTPUTS)
        {
            response["MsgType"] = MSG_HEARTBEAT_RESPONSE;
            for(JsonObject o : request[OUT_BUTTONS].as<JsonArray>()){
                outButtonData[o["Index"].as<int>()] = o["Value"].as<bool>();
            }
            for(JsonObject o : request[OUT_AXES].as<JsonArray>()){
                outAxisData[o["Index"].as<int>()] = o["Value"].as<short>();
            }
            for(JsonObject o : request[OUT_DELTAS].as<JsonArray>()){
                outDeltaData[o["Index"].as<int>()] = o["Value"].as<short>();
            }
            for(JsonObject o : request[OUT_EVENTS].as<JsonArray>()){
                outEventData[o["Index"].as<int>()] = o["Value"].as<bool>();
            }
            if((_lastUpdateMillis - _lastSendMillis) < UCR_KEEPALIVE_MS){
                // If we have just talked to the other party, don't send a new response
                return true;
            }
            // TODO: Add ACK messages, to ensure delivery? This might not really matter, as the state will be broadcast again soon
        }

        if( msgType == MSG_SUBSCRIBE_TO_INPUTS)
        {
            response["MsgType"] = MSG_HEARTBEAT_RESPONSE;

            // Note details about subscriber
            _remoteIP = Udp.remoteIP();
            _remotePort = Udp.remotePort();
            _subscribed = true;

            D(Serial.printf("Registered subscriber: %s:%d\n", _remoteIP.toString().c_str(), _remotePort));
        }

        if( msgType == MSG_UNSUBSCRIBE_FROM_INPUTS)
        {
            response["MsgType"] = MSG_HEARTBEAT_RESPONSE;

            D(Serial.printf("Unregistered subscriber: %s:%d\n", _remoteIP.toString().c_str(), _remotePort));
            _subscribed = false;
        }

        if( msgType == MSG_HEARTBEAT_RESPONSE)
        {
            // Don't respond to heartbeat responses - return early
            return true;
        }

        if( msgType == MSG_BIND_START)
        {
            _remoteIP = Udp.remoteIP();
            _remotePort = Udp.remotePort();
            _bindMode = true;
            D(Serial.printf("Bind started: %s:%d\n", _remoteIP.toString().c_str(), _remotePort));
            return true;
        }

        if( msgType == MSG_BIND_STOP)
        {
            _bindMode = false;
            D(Serial.printf("Bind stopped: %s:%d\n", _remoteIP.toString().c_str(), _remotePort));
            return true;
        }

        response["seq"] = _sequenceNo++;
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        serializeMsgPack(response, Udp);
        Udp.endPacket();
        _lastSendMillis = _lastUpdateMillis;

        D(Serial.print("TX1: "));
        D(serializeJson(response, Serial));
        D(Serial.println());

        return true;
    }
    return false;
}

void UCR::setSubscriber(IPAddress address, uint16_t port){
    _remoteIP = address;
    _remotePort = port;
    D(Serial.printf("Registered subscriber: %s:%d\n", _remoteIP.toString().c_str(), _remotePort));
}

void addInput(JsonDocument *doc, const char *name, const char **list, bool *value, int size)
{
    JsonArray input_array = doc->createNestedArray(name);
    for (int i = 0; i < size; i++)
    {
        if (list[i] != nullptr)
        {
            input_array.add(value[i] ? 0 : 1);
        }
    }
}

void addInput(JsonDocument *doc, const char *name, const char **list, short *value, int size)
{
    JsonArray input_array = doc->createNestedArray(name);
    for (int i = 0; i < size; i++)
    {
        if (list[i] != nullptr)
        {
            input_array.add(value[i]);
        }
    }
}

bool UCR::sendBindResponse(int index, char *category, short value)
{
    StaticJsonDocument<500> response;
    response["MsgType"] = MSG_BIND_RESPONSE;
    response["hostname"] = _hostString;
    response["index"] = index;
    response["category"] = category;
    response["value"] = value;


    response["seq"] = _sequenceNo++;
    Udp.beginPacket(_remoteIP, _remotePort);
    serializeMsgPack(response, Udp);
    Udp.endPacket();

    _lastSendMillis = millis();

    D(Serial.print("TX2: "));
    D(serializeJson(response, Serial));
    D(Serial.println());
}

bool UCR::updateSubscriber()
{
    if(!_subscribed) return false;

    StaticJsonDocument<500> response;
    response["MsgType"] = MSG_UPDATE_SUBSCRIBER;
    response["hostname"] = _hostString;

    addInput(&response, IN_BUTTONS, _inButtonList, inButtonData, UCR_IN_BUTTON_COUNT);
    addInput(&response, IN_AXES, _inAxisList, inAxisData, UCR_IN_AXIS_COUNT);
    addInput(&response, IN_DELTAS, _inDeltaList, inDeltaData, UCR_IN_DELTA_COUNT);
    addInput(&response, IN_EVENTS, _inEventList, inEventData, UCR_IN_EVENT_COUNT);

    response["seq"] = _sequenceNo++;
    Udp.beginPacket(_remoteIP, _remotePort);
    serializeMsgPack(response, Udp);
    Udp.endPacket();

    _lastSendMillis = millis();

    D(Serial.print("TX3: "));
    D(serializeJson(response, Serial));
    D(Serial.println());

    _inputDirty = 0;

    return true;
}
