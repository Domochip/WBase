#include "MQTTMan.h"

void MQTTMan::prepareTopic(String &topic)
{
    if (topic.indexOf(F("$sn$")) != -1)
    {
        char sn[9];
#ifdef ESP8266
        sprintf_P(sn, PSTR("%08x"), ESP.getChipId());
#else
        sprintf_P(sn, PSTR("%08x"), (uint32_t)(ESP.getEfuseMac() << 40 >> 40));
#endif
        topic.replace(F("$sn$"), sn);
    }

    if (topic.indexOf(F("$mac$")) != -1)
        topic.replace(F("$mac$"), WiFi.macAddress());

    if (topic.indexOf(F("$model$")) != -1)
        topic.replace(F("$model$"), CUSTOM_APP_MODEL);

    // check for final slash
    if (topic.length() && topic.charAt(topic.length() - 1) != '/')
        topic += '/';
}

bool MQTTMan::connect(bool firstConnection)
{
    char sn[9];
#ifdef ESP8266
    sprintf_P(sn, PSTR("%08x"), ESP.getChipId());
#else
    sprintf_P(sn, PSTR("%08x"), (uint32_t)(ESP.getEfuseMac() << 40 >> 40));
#endif

    // generate clientID
    String clientID(F(CUSTOM_APP_MODEL));
    clientID += sn;

    // Connect
    char *username = (_username[0] ? _username : nullptr);
    char *password = (_username[0] ? _password : nullptr);
    char *willTopic = (_connectedAndWillTopic[0] ? _connectedAndWillTopic : nullptr);
    const char *willMessage = (_connectedAndWillTopic[0] ? "0" : nullptr);
    MQTTClient::setWill(willTopic, willMessage, true, 0);
    MQTTClient::connect(clientID.c_str(), username, password);

    if (connected())
    {
        if (_connectedAndWillTopic[0])
            publish(_connectedAndWillTopic, "1", true);

        // Subscribe to needed topic
        if (_connectedCallBack)
            _connectedCallBack(this, firstConnection);
    }

    return connected();
}

MQTTMan &MQTTMan::setConnectedAndWillTopic(const char *topic)
{
    if (!topic)
        _connectedAndWillTopic[0] = 0;
    else if (strlen(topic) < sizeof(_connectedAndWillTopic))
        strcpy(_connectedAndWillTopic, topic);

    return *this;
}

MQTTMan &MQTTMan::setConnectedCallback(CONNECTED_CALLBACK_SIGNATURE connectedCallback)
{
    _connectedCallBack = connectedCallback;
    return *this;
}

MQTTMan &MQTTMan::setDisconnectedCallback(DISCONNECTED_CALLBACK_SIGNATURE disconnectedCallback)
{
    _disconnectedCallBack = disconnectedCallback;
    return *this;
}

bool MQTTMan::connect(const char *username, const char *password)
{
    // check logins
    if (username && strlen(username) >= sizeof(_username))
        return false;
    if (password && strlen(password) >= sizeof(_password))
        return false;

    if (username)
        strcpy(_username, username);
    else
        _username[0] = 0;

    if (password)
        strcpy(_password, password);
    else
        _password[0] = 0;

    return connect(true);
}

void MQTTMan::disconnect()
{
    // publish disconnected just before disconnect...
    if (_connectedAndWillTopic[0])
        publish(_connectedAndWillTopic, "0", true);

    // Stop MQTT Reconnect
    _mqttReconnectTicker.detach();
    // Disconnect
    if (connected() && MQTTClient::disconnect())
    {
        // call disconnected callback
        if (_disconnectedCallBack)
            _disconnectedCallBack();
    }
}

bool MQTTMan::publishToConnectedTopic(const char *payload)
{
    if (_connectedAndWillTopic[0])
        return publish(_connectedAndWillTopic, payload, true);
    return false;
}

String MQTTMan::getStateString()
{
    //TODO complete with all lwmqtt_err_t or choose relevant ones
    switch (lastError())
    {
    case LWMQTT_SUCCESS:
        return F("Success");
    case LWMQTT_BUFFER_TOO_SHORT:
        return F("Buffer Too Short");
    case LWMQTT_VARNUM_OVERFLOW:
        return F("VarNum Overflow");
    case LWMQTT_NETWORK_FAILED_CONNECT:
        return F("Network Failed Connect");
    case LWMQTT_NETWORK_TIMEOUT:
        return F("Network Timeout");
    case LWMQTT_NETWORK_FAILED_READ:
        return F("Network Failed Read");
    case LWMQTT_NETWORK_FAILED_WRITE:
        return F("Network Failed Write");
    case LWMQTT_REMAINING_LENGTH_OVERFLOW:
        return F("Remaining Length Overflow");
    case LWMQTT_REMAINING_LENGTH_MISMATCH:
        return F("Remaining Length Mismatch");
    case LWMQTT_MISSING_OR_WRONG_PACKET:
        return F("Missing or Wrong Packet");
    default:
        return F("Unknown");
    }
}

bool MQTTMan::loop()
{
    if (lastError() != LWMQTT_SUCCESS) // TODO to be verified for reliability
    {
        // evaluate connection status and call disconnected callback if needed
        // if we are not connected, reconnect ticker not started nor _needMqttReconnect flag raised and disconnected callback set
        if (!connected() && !(_mqttReconnectTicker.active() || _needMqttReconnect) && _disconnectedCallBack)
            _disconnectedCallBack();

        if (_needMqttReconnect)
        {
            _needMqttReconnect = false;

            LOG_SERIAL_PRINT(F("MQTT Reconnection : "));

            bool res = connect(false);

            LOG_SERIAL_PRINTLN(res ? F("OK") : F("Failed"));
        }

        // if not connected and reconnect ticker not started
        if (!connected() && !_mqttReconnectTicker.active())
        {
            LOG_SERIAL_PRINTLN(F("MQTT Disconnected"));
            // set Ticker to reconnect after 20 or 60 sec (Wifi connected or not)
#ifdef ESP8266
            _mqttReconnectTicker.once((WiFi.isConnected() ? 20 : 60), [this]()
                                      { _needMqttReconnect = true; });

#else
            _mqttReconnectTicker.once<typeof this>((WiFi.isConnected() ? 20 : 60), [](typeof this mqttMan)
                                                   { mqttMan->_needMqttReconnect = true; }, this);
#endif
        }

        return MQTTClient::loop();
    }
    return true;
}