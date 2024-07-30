# MQTT Connection with SIM7600 Using AT Commands

This project demonstrates how to connect to an MQTT broker using the SIM7600 module with AT commands. The code consists of two main functions: `Init` and `getMQTT`. The `Init` function establishes a network and GPRS connection, while the `getMQTT` function connects to an MQTT broker and sends messages.

## AT Commands to Connect to Network and GPRS

1. **AT+CFUN=1** - Set phone functionality.
2. **AT+CPIN?** - Check if PIN is required.
3. **AT+CSQ** - Signal quality report.
4. **AT+CREG?** - Network registration status.
5. **AT+COPS?** - Operator selection.
6. **AT+CGATT?** - GPRS attach or detach status.
7. **AT+CPSI?** - Serving cell information.
8. **AT+CGDCONT=1,"IP","dialogbb"** - Define PDP context.
9. **AT+CGACT=1,1** - Activate PDP context.
10. **AT+CGATT?** - GPRS attach or detach status (repeated).
11. **AT+CGPADDR=1** - Show PDP address.
12. **AT+NETOPEN** - Open network service.
13. **AT+NETSTATE** - Query network state.

## AT Commands to Connect to MQTT

1. **AT+CMQTTSTART** - Start MQTT service.
2. **AT+CMQTTACCQ=<client_index>,<clientID>,<server_type>** - Acquire a client.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<clientID>` - Unique client name. String length of 1-23 bytes.
   - `<server_type>` - Identifies server type. Default is 0.
3. **AT+CMQTTWILLTOPIC=<client_index>,<req_length>** - Input the topic of will message.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<req_length>` - Length of input topic. Range is from 1-1024 bytes.
4. **AT+CMQTTWILLMSG=<client_index>,<req_length>,<qos>** - Input the will message.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<req_length>` - Length of input data. Range is from 1-10240 bytes.
   - `<qos>` - QoS value of will message. Range is from 0-2.
5. **AT+CMQTTCONNECT=<client_index>,<server_addr>,<keepalive_time>,<clean_session>,<user_name>,<pass_word>** - Connect to MQTT server.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<server_addr>` - String describing server address and port.
   - `<keepalive_time>` - Time interval between two messages received from a client. Range is from 60s to 64800s (18h).
   - `<clean_session>` - Clean session flag. Value range is from 0 to 1. Default value is 0.
   - `<user_name>` - Identifies name of user for authentication when connecting to server. String length is 1-256.
   - `<pass_word>` - Password corresponding to user for authentication when connecting to server. String length is 1-256.
6. **AT+CMQTTTOPIC=<client_index>,<req_length>** - Input the topic of publish message.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<req_length>` - Length of input topic data. Range is from 1 to 1024 bytes.
7. **AT+CMQTTPAYLOAD=<client_index>,<req_length>** - Input the publish message.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<req_length>` - Length of input data. Range is from 1-10240 bytes.
8. **AT+CMQTTPUB=<client_index>,<qos>,<pub_timeout>** - Publish a message to server.
   - `<client_index>` - Identifies a client. Permitted values are 0 or 1.
   - `<qos>` - QoS value of publish message. Range is from 0-2.
   - `<pub_timeout>` - Publishing timeout interval. Range is from 60s to 180s.

