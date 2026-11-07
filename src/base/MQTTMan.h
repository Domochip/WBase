#ifndef MQTTMan_h
#define MQTTMan_h

#include "../Main.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <Ticker.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MQTTMan : private PubSubClient
{
public:
    using ConnectedCallback = std::function<void(MQTTMan * mqttMan, bool firstConnection)>;
    using DisconnectedCallback = std::function<void()>;

private:
    // Credentials are copied here by connect(username, password) and reused for every reconnection.
    // Size = 64 characters + terminating '\0': 64 is the longest value the configuration pages of the
    // applications accept (e.g. WPalaControl: MQTT password field maxlength='64', stored in char[64 + 1]).
    // With a 64-byte buffer, a 64-character password was accepted by the page and then silently rejected
    // here, and MQTT never connected.
    static constexpr size_t credentialMaxLength = 64;
    char _username[credentialMaxLength + 1] = {0};
    char _password[credentialMaxLength + 1] = {0};
    // set by connect(username, password) when a credential doesn't fit: no connection is attempted,
    // so the state stays MQTT_DISCONNECTED and getStateString() reports the real reason
    bool _credentialsTooLong = false;
    char _connectedAndWillTopic[96] = {0};
    char _baseTopic[64 + 4 + 12 - 7 + sizeof(CUSTOM_APP_MODEL) + 1] = {0};
    bool _needMqttReconnect = false;
    Ticker _mqttReconnectTicker;
    uint16_t _connectionCount = 0;
    int8_t _lastDiscoState = 0;   // last MQTT disconnection state code (0 = no disconnection yet)

    ConnectedCallback _connectedCallBack = nullptr;
    DisconnectedCallback _disconnectedCallBack = nullptr;

    bool connect(bool firstConnection);

public:
    static constexpr size_t baseTopicSize = 64 + 4 + 12 - 7 + sizeof(CUSTOM_APP_MODEL) + 1; // usual baseTopic length + one of each placeholder (+4 sn) (+12 mac) (+X model)
    static void prepareTopic(const char *topicTemplate, char *result, size_t resultSize);
    MQTTMan &setBaseTopic(const char *baseTopicTemplate);
    const char *getBaseTopic() const;

    using PubSubClient::setClient;
    using PubSubClient::setServer;
    using PubSubClient::setKeepAlive;
    MQTTMan &setConnectedAndWillTopic(const char *topic);
    MQTTMan &setConnectedCallback(ConnectedCallback connectedCallback);
    MQTTMan &setDisconnectedCallback(DisconnectedCallback disconnectedCallback);
    using PubSubClient::setCallback;
    bool connect(const char *username = nullptr, const char *password = nullptr);
    using PubSubClient::connected;
    void disconnect();
    using PubSubClient::beginPublish;
    using PubSubClient::endPublish;
    using PubSubClient::publish;
    bool publish(const char *topic, JsonVariantConst payload, bool retained = false);
    bool publishToConnectedTopic(const char *payload);
    using PubSubClient::publish_P;
    using PubSubClient::state;
    const __FlashStringHelper *getStateString();
    uint16_t getConnectionCount() const;
    int8_t getLastDiscoState() const;
    using PubSubClient::getBufferSize;
    using PubSubClient::setBufferSize;
    using PubSubClient::subscribe;
    bool loop();
};

#endif