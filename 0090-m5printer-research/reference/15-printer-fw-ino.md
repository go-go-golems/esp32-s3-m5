1

2

3

4

5

6

7

8

9

10

11

12

13

14

15

16

17

18

19

20

21

22

23

24

25

26

27

28

29

30

31

32

33

34

35

36

37

38

39

40

41

42

43

44

45

46

47

48

49

50

51

52

53

54

55

56

57

58

59

60

61

62

63

64

65

66

67

68

69

70

71

72

73

74

75

76

77

78

79

80

81

82

83

84

85

86

87

88

89

90

91

92

93

94

95

96

97

98

99

100

101

102

103

104

105

106

107

108

109

110

111

112

113

114

115

116

117

118

119

120

121

122

123

124

125

126

127

128

129

130

131

132

133

134

135

136

137

138

139

140

141

142

143

144

145

146

147

148

149

150

151

152

153

154

155

156

157

158

159

160

161

162

163

164

165

166

167

168

169

170

171

172

173

174

175

176

177

178

179

180

181

182

183

184

185

186

187

188

189

190

191

192

193

194

195

196

197

198

199

200

201

202

203

204

205

206

207

208

209

210

211

212

213

214

215

216

217

218

219

220

221

222

223

224

225

226

227

228

229

230

/\*

\* SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD

\*

\* SPDX-License-Identifier: MIT

\*/

/\*\*

\* Please install the following dependent libraries before compiling:

\* M5Atom: https://github.com/m5stack/M5Atom

\* FastLED: https://github.com/FastLED/FastLED

\* PubSubClient: https://github.com/knolleary/pubsubclient

\* ArduinoJson: https://github.com/bblanchon/ArduinoJson

\* @Hardwares: Atom Printer

\* @Platform Version: Arduino M5Stack Board Manager v2.1.4

\*/

/\*

How to use:

1\. connect to AP \`ATOM\_PRINTER-xxxx\`

2\. Visit 192.168.4.1 to print

3\. Configure WiFi connection and print data through mqtt server (refer README)

\*/

#include <M5Atom.h>

#include "ATOM\_PRINTER.h"

#include "ATOM\_PRINTER\_CONFIG.h"

#include "ATOM\_PRINTER\_WEB.h"

#include "ATOM\_PRINTER\_MQTT.h"

#include "ATOM\_PRINTER\_WIFI.h"

#include <Preferences.h>

#include <PubSubClient.h>

#include <WiFi.h>

#include <WiFiClient.h>

#include <ArduinoJson.h>

xSemaphoreHandle xMQTTMutex = xSemaphoreCreateMutex();

Preferences preferences;

ATOM\_PRINTER printer;

DynamicJsonDocument payload(1024);

WebServer webServer(80);

DNSServer dnsServer;

const IPAddress apIP(192, 168, 4, 1);

uint8\_t bmp\_buffer\[BMP\_BUFFER\_LIMIT\] = {0};

int bmp\_data\_offset = 0;

int bmp\_data\_size = 0;

int bmp\_width = 0;

int bmp\_height = 0;

// wifi设置

const char \*apSSID = "ATOM\_PRINTER";

String wifi\_ssid = "";

String wifi\_password = "";

String ssid\_html;

bool is\_config\_mode = true;

String device\_mac;

// mqtt

String mqtt\_broker = MQTT\_BROKER;

int mqtt\_port = MQTT\_PORT;

String mqtt\_id = MQTT\_ID;

String mqtt\_user = MQTT\_USER;

String mqtt\_password = MQTT\_PASSWORD;

String mqtt\_topic = MQTT\_TOPIC;

bool mqtt\_connect\_change\_event = false;

WiFiClient client;

PubSubClient mqttClient(client);

Atom\_Printer\_State\_t device\_state = kInit;

void flashing(uint32\_t color, uint8\_t frequency)

{

static uint32\_t prev\_ms = millis();

static bool rgbState = 0;

if (millis() > prev\_ms + frequency) {

prev\_ms = millis();

rgbState =!rgbState;

}

M5.dis.drawpix(0, color \* rgbState);

}

void TaskLED(void \*pvParameters)

{

while (1) {

switch (device\_state) {

case kInit:

// M5.dis.drawpix(0, 0xffe500); //yellow

flashing(0x00ff00, 20); // blinking green

break;

case kWiFiConnected:

M5.dis.drawpix(0, 0x00ff00); // green

break;

case kWiFiDisconnected:

flashing(0xff0000, 20); // blinking red

break;

case kMQTTConnected:

M5.dis.drawpix(0, 0x0000ff); // blue

break;

case kMQTTDisconnected:

flashing(0x0000ff, 20); // blinking blue

break;

}

vTaskDelay(500);

}

}

void mqttCallback(char \*topic, byte \*payload, unsigned int len)

{

// mqtt回调函数：将从订阅主题获得的信息通过串口打印

char PayloadData\[len + 1\];

String Type = "";

int posx;

uint8\_t indexs;

uint8\_t fonts;

strncpy(PayloadData, (char \*)payload, len);

PayloadData\[len\] = '\\0';

Serial.println(mqtt\_topic + ":");

Serial.printf("leng:%d\\r\\n", len);

Serial.println(String(PayloadData));

Type = String(PayloadData);

if (Type.indexOf("TEXT") >= 0) {

Type = Type.substring(5);

posx = Type.toInt();

indexs = Type.indexOf(",");

Type = Type.substring(indexs + 1);

fonts = Type.toInt();

indexs = Type.indexOf(":");

printer.init();

printer.printPos(posx);

printer.fontSize(fonts);

printer.printASCII(&Type\[indexs + 1\]);

printer.newLine(3);

} else if (Type.indexOf("QR:") >= 0) {

printer.init();

printer.printQRCode(&Type\[3\]);

printer.newLine(3);

} else if (Type.indexOf("BAR:") >= 0) {

printer.init();

printer.setBarCodeHRI(HIDE);

printer.printBarCode(CODE128, &Type\[4\]);

printer.newLine(3);

}

}

void setup()

{

M5.begin(true, false, true);

printer.begin();

M5.dis.drawpix(0, 0x00ffff); // 初始化状态灯

preferences.begin("PRINTER\_CONFIG");

// disableCore0WDT();

printer.init();

// printer.newLine(1);

// Create LED Task

xTaskCreatePinnedToCore(TaskLED, "TaskLED" // A name just for humans

,

2048 // This stack size can be checked & adjusted

// by reading the Stack Highwater

,

NULL,

3 // Priority, with 3 (configMAX\_PRIORITIES - 1)

// being the highest, and 0 being the lowest.

,

NULL, 0);

wifiInit();

ssid\_html = wifiScan();

webServerInit();

device\_mac = WiFi.softAPmacAddress();

mqttClient.setBufferSize(4096);

mqttClient.setCallback(mqttCallback);

mqttClient.setKeepAlive(10);

if (preferences.getString("WIFI\_SSID").length() > 1) {

Serial.println(wifi\_ssid);

wifi\_ssid = preferences.getString("WIFI\_SSID");

wifi\_password = preferences.getString("WIFI\_PWD");

Serial.println(wifi\_ssid);

Serial.println(wifi\_password);

Serial.println("Get WIFI INFO From Preference");

}

Serial.println(mqtt\_broker);

}

void loop()

{

webServer.handleClient();

dnsServer.processNextRequest();

if (WiFi.status() == WL\_CONNECTED) {

if (!mqttClient.connected()) {

Serial.println("reconnect mqtt...");

// xSemaphoreTake(xMQTTMutex, portMAX\_DELAY);

mqttConnect(mqtt\_broker, mqtt\_port, mqtt\_id, mqtt\_user, mqtt\_password, 2000);

// mqtt\_connect\_change\_event = false;

// xSemaphoreGive(xMQTTMutex);

} else {

mqttClient.loop();

}

} else {

if (wifi\_ssid!= "") {

wifiConnect(wifi\_ssid, wifi\_password, 5000);

}

}

if (M5.Btn.pressedFor(5000)) {

preferences.clear();

Serial.println("reset device...");

esp\_restart();

}

M5.update();

}

// print bmp

// printer.init();x

// printer.printASCII("M5STACK");

// delay(2000);

// printer.init();

// printer.setBarCodeHRI(ABOVE);

// printer.printBarCode(CODE128, "M5STACK");

// delay(2000);

// printer.init();

// printer.printQRCode("M5STACK");

// delay(2000);

// printer.init();

// printer.printBMP(0, 184, 180, bitbuffer2);