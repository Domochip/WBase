#ifndef MQTTMan_h
#define MQTTMan_h

#include "../Main.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <Ticker.h>
#include <MQTT.h>

#define CONNECTED_CALLBACK_SIGNATURE std::function<void(MQTTMan * mqttMan, bool firstConnection)>
#define DISCONNECTED_CALLBACK_SIGNATURE std::function<void()>

class MQTTMan : private MQTTClient
{
private:
    char _username[64] = {0};
    char _password[64] = {0};
    char _connectedAndWillTopic[96] = {0};
    bool _needMqttReconnect = false;
    Ticker _mqttReconnectTicker;

    CONNECTED_CALLBACK_SIGNATURE _connectedCallBack = nullptr;
    DISCONNECTED_CALLBACK_SIGNATURE _disconnectedCallBack = nullptr;

    bool connect(bool firstConnection);

public:
    static void prepareTopic(String &topic);

    using MQTTClient::begin;
    MQTTMan &setConnectedAndWillTopic(const char *topic);
    MQTTMan &setConnectedCallback(CONNECTED_CALLBACK_SIGNATURE connectedCallback);
    MQTTMan &setDisconnectedCallback(DISCONNECTED_CALLBACK_SIGNATURE disconnectedCallback);
    using MQTTClient::onMessageAdvanced;
    bool connect(const char *username = nullptr, const char *password = nullptr);
    using MQTTClient::connected;
    void disconnect();
    using MQTTClient::publish;
    bool publishToConnectedTopic(const char *payload);
    String getStateString();
    using MQTTClient::subscribe;
    bool loop();

    MQTTMan(int readBufSize, int writeBufSize) : MQTTClient(readBufSize, writeBufSize) {};
};

#endif