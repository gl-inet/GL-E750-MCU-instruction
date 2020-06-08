**Since the microcontroller only recognizes the string type in the JSON format, the following parameters, even if the parameter type is INT, will be converted to a string and sent**
###WIFI Related

| Parameter name | Type | Necessity | Default | Description                                            | Possible value                                |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|ssid|string|yes|" "|2G WiFi SSID|A string of up to 32 characters|
|up|string|yes|"0"|Indicates whether 2G WIFI is enabled. If it is not enabled, the LCD will not display the 2G WIFI page.|0 or 1|
|key|string|no|" "|2G WiFi password, if it is empty, it means no encryption, LCD shows OPEN|A string of up to 64 characters|
|ssid_5g|string|yes|" "|5G WiFi SSID|A string of up to 32 characters|
|up_5g|string|yes|"0"|Indicates whether 5G WIFI is enabled. If it is not enabled, the LCD will not display the 5G WIFI page.|0 or 1|
|key_5g|string|no|" "|5G WiFi password, if it is empty, it means no encryption, LCD shows OPEN|A string of up to 64 characters|
|hide_psk|string|no|"0"|Whether to hide the wifi password on the LCD|0 or 1|

###Modem related

| Parameter name |  Type  | Necessity | Default  | Description                                                  | Possible value                                               |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|SIM|string|no|"NO_SIM"| SIM card status, there is no SIM parameter normally, if there is SIM parameter, other parameters will not be transferred | NO_SIM (No SIM card detected), PIN_SIM (PIN code required), NO_REG (No service) |
|carrier|string|no|"0"|Carrier name|A string of up to 16 characters|
|sms|string|no|"0"|/Number of text messages. If this parameter is greater than 0, the LCD displays the text message icon.|Numbers greater than 0|
|signal|string|no|"0"|Signal strength|0~4|
|modem_mode|string|no|" "|Network mode|2G，3G, 4G, 4G+|
|modem_up|string|no|"0"|Whether the modem data is enabled|0 or 1|

###Network Related

| Parameter name |  Type  | Necessity | Default | Description                                                  | Possible value                                               |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|work_mode|string|yes|" "|Router network mode|Router,AP,WDS,Extender|
|lan_ip|string|yes|" "| Router gateway address, or the IP address of the router under bridge mode |Legal IP address|
|method_nw|string|yes|" "|Router's current Internet access| cable,repeater,modem,tethering, if there is extra information, use "\|" to separate them. For example, repeater&#124;GL-AR750S-081 |

###VPN related

| Parameter name |  Type  | Necessity | Default | Description            | Possible value                   |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|vpn_type|string|yes|" "|VPN protocol|openvpn,wireguard|
|vpn_status|string|yes|" "|VPN connection status|connected，connecting，off|
|vpn_server|string|yes|" "|VPN configuration name|A string of up to 128 characters|

###Client related

| Parameter name |  Type  | Necessity | Default | Description       | Possible value                     |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|clients|string|yes|"0"|Number of clients|Numbers greater than or equal to 0|

###customization related

| Parameter name |  Type  | Necessity | Default | Description                                                  | Possible value                  |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
| display_mask | string | no | "1f" | This value indicates whether the 1-5 screen is displayed. You need to convert this value to the corresponding binary when setting. For example, 0x03 converted to binary is 00011, which means that only the first screen and the second screen are displayed; the default 1f, that is, 11111, displays 5 screen contents |0x0-0x1f|
| custom_en | string | no | "0" | This value indicates whether the user is using a custom page, 0 means not use, 1 means use |0 or 1|
| content | string| no |" "| Display content | A string of up to 64 characters |
|msg|string|no|" "|Display content on the screen for 20 seconds|A string of up to 64 characters|

###system related

| Parameter name |  Type  | Necessity | Default | Description                                   | Possible value                                               |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|button|string|no|"0"|The time the reset button was pressed|Numbers greater than or equal to 0|
|system|string|no|"boot"|Show system status on screen|reboot (reboot), reft (restore factory settings), adding (system upgrade), gouboot (enter uboot mode), boot (boot), Calibrate stage (calibration stage), Flash stage (waiting to upgrade standard firmware stage), Test stage 1 (Test Phase 1), Test stage 2 （Test Phase 2）|
|disk|string|no|"0"| Is there a disk                               |0 or 1|
|tor|string|no|"0"|Is it the Tor firmware|0 or 1|
|debug|string|no|"0"|Whether to print debug information in logread|0 or 1|

###MCU status related

| Parameter name |  Type  | Necessity | Default | Description                                                  | Possible value |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|mcu_status|string|no|NO|Get the status of the microcomputer, send the command within 1 second, the microcomputer will return the relevant data through the serial port, which are the percentage of power, the temperature of the coulometer, the state of charge, the number of battery charging cycles, and the battery voltage|  |

###screen test

| Parameter name |  Type  | Necessity | Default | Description                    | Possible value                            |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|lcd_test|string|no|NO|Test the screen for bad pixels|1 (light up all pixels) or 0 (off screen)|

###coulometer parameter query

| Parameter name |  Type  | Necessity | Default | Description                                            | Possible value |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|QEN|string|no|NO| Check if the coulometer algorithm is enabled ||
|chemid|string|no|NO| Check coulometer file version                          ||
|high_temp|string|no|72| Set high temperature shutdown value, don't set too low ||

###MCU firmware version query

| Parameter name |  Type  | Necessity | Default | Description                | Possible value |
| :----------: | :-----: | :----------------: | ----------------- | ----------------- | ----------------- |
|version|string|no|NO| Check MCU firmware version ||

###Application example
**Use the echo command directly to send data in json format to the system serial port. This example contains basic WIFI information, SIM card information, VPN status, client status, time, etc. The MCU_status parameter is included in the example, which indicates that the microcontroller is required to return status**

```
echo '{ "ssid_5g": "GL-E750-719", "up_5g": "1", "key_5g": "goodlife", "ssid": "GL-E750-719", "up": "1", "key": "goodlife", "SIM": "NO_SIM", "work_mode": "Router", "lan_ip": "192.168.82.1",  "vpn_status": "off", "clients": "1", "clock": "02:30", "mcu_status": "1" }' >/dev/ttyS0
```
**After the command is executed, the serial port will return the status within 1 second. The return value is as follows. Each parameter is separated by a comma, where {OK} indicates successful execution, 99 indicates that 99% of the current power is left, and 42.4 indicates that the current coulometer Temperature, 1 means charging, 2 means the battery has two charge and discharge cycles****

```
{OK},99,42.4,1,2
```
