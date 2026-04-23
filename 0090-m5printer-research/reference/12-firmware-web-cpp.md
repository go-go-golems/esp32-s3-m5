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

231

232

233

234

235

236

237

238

239

240

241

242

243

244

245

246

247

248

249

250

251

252

253

254

255

256

257

258

259

260

261

262

263

264

265

266

267

268

269

270

271

272

273

274

275

276

277

278

279

280

281

282

283

284

285

286

287

288

289

290

291

292

293

294

295

296

297

298

299

300

301

302

303

304

305

306

307

308

309

310

311

312

313

314

315

316

317

318

319

320

321

322

323

324

325

326

327

328

329

330

331

332

333

334

335

336

337

338

339

340

341

342

343

344

345

346

347

348

349

350

351

352

353

354

355

356

357

#include "ATOM\_PRINTER.h"

#include "ATOM\_PRINTER\_WEB.h"

#include "ATOM\_PRINTER\_CONFIG.h"

#include "ATOM\_PRINTER\_WIFI.h"

#include "ATOM\_PRINTER\_HTML.h"

#include "ATOM\_PRINTER\_MQTT.h"

#include <Preferences.h>

#include <ArduinoJson.h>

extern Preferences preferences;

extern ATOM\_PRINTER printer;

extern String ssid\_html;

extern WebServer webServer;

extern DNSServer dnsServer;

extern DynamicJsonDocument payload;

extern String mqtt\_topic;

extern uint8\_t bmp\_buffer\[BMP\_BUFFER\_LIMIT\];

extern int bmp\_data\_offset;

extern int bmp\_data\_size;

extern int bmp\_width;

extern int bmp\_height;

extern PubSubClient mqttClient;

extern xSemaphoreHandle xMQTTMutex;

// 保存网页获取的数据

String Pdata, newLine, QRCode, AdjustLevel, printType, BarCode, BarType, Position;

String urlDecode(String input)

{

String s = input;

s.replace("%20", " ");

s.replace("+", " ");

s.replace("%21", "!");

s.replace("%22", "\\"");

s.replace("%23", "#");

s.replace("%24", "$");

s.replace("%25", "%");

s.replace("%26", "&");

s.replace("%27", "\\'");

s.replace("%28", "(");

s.replace("%29", ")");

s.replace("%30", "\*");

s.replace("%31", "+");

s.replace("%2C", ",");

s.replace("%2E", ".");

s.replace("%2F", "/");

s.replace("%2C", ",");

s.replace("%3A", ":");

s.replace("%3A", ";");

s.replace("%3C", "<");

s.replace("%3D", "=");

s.replace("%3E", ">");

s.replace("%3F", "?");

s.replace("%40", "@");

s.replace("%5B", "\[");

s.replace("%5C", "\\\\");

s.replace("%5D", "\]");

s.replace("%5E", "^");

s.replace("%5F", "-");

s.replace("%60", "\`");

return s;

}

void handleRoot()

{

webServer.send(200, "text/html", (char\*)printer\_html);

}

void handleWiFiConfig()

{

Serial.println("Handling WiFi config request");

// 检查是否有请求体

if (!webServer.hasArg("plain")) {

webServer.send(400, "text/plain", "Bad Request: No JSON data");

return;

}

// 解析JSON数据

DynamicJsonDocument doc(512);

DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

if (error) {

Serial.print("JSON parse failed: ");

Serial.println(error.c\_str());

webServer.send(400, "text/plain", "Bad Request: Invalid JSON");

return;

}

String ssid = doc\["ssid"\].as<String>();

String password = doc\["password"\].as<String>();

Serial.println("Received WiFi config:");

Serial.print("SSID: ");

Serial.println(ssid);

Serial.print("Password: ");

Serial.println(password);

// 验证SSID

if (ssid.length() == 0) {

webServer.send(400, "text/plain", "Error: SSID cannot be empty");

return;

}

// 尝试连接WiFi

bool connectSuccess = wifiConnect(ssid, password, 5000);

if (connectSuccess) {

webServer.send(200, "text/plain", "OK");

Serial.println("WiFi connected successfully");

} else {

webServer.send(500, "text/plain", "Error: Failed to connect to WiFi");

Serial.println("WiFi connection failed");

}

}

void handleStatusConfig()

{

Serial.println("Handling status request");

DynamicJsonDocument doc(1024);

// WiFi状态

if (WiFi.status() == WL\_CONNECTED) {

doc\["WIFI\_STATE"\] = true;

doc\["SSID"\] = WiFi.SSID();

doc\["IP"\] = WiFi.localIP().toString();

doc\["RSSI"\] = WiFi.RSSI();

// MQTT状态

xSemaphoreTake(xMQTTMutex, portMAX\_DELAY);

if (mqttClient.connected()) {

doc\["MQTT\_STATE"\] = "Connected";

doc\["MQTT\_BROKER"\] = mqtt\_broker;

doc\["MQTT\_TOPIC"\] = mqtt\_topic;

} else {

doc\["MQTT\_STATE"\] = "Disconnected";

}

xSemaphoreGive(xMQTTMutex);

} else {

doc\["WIFI\_STATE"\] = false;

doc\["MQTT\_STATE"\] = "Disconnected";

}

// 可用的WiFi网络列表

doc\["WIFI\_HTML"\] = ssid\_html;

// 发送响应

String response;

serializeJson(doc, response);

webServer.send(200, "application/json", response);

Serial.println("Status response sent");

}

void handleMQTTConfig()

{

Serial.println("Handling MQTT config request");

// 检查是否有请求体

if (!webServer.hasArg("plain")) {

webServer.send(400, "text/plain", "Bad Request: No JSON data");

return;

}

// 解析JSON数据

DynamicJsonDocument doc(512);

DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

if (error) {

Serial.print("JSON parse failed: ");

Serial.println(error.c\_str());

webServer.send(400, "text/plain", "Bad Request: Invalid JSON");

return;

}

String broker = doc\["mqtt\_broker"\].as<String>();

int port = doc\["mqtt\_port"\].as<int>();

String id = doc\["mqtt\_id"\].as<String>();

String user = doc\["mqtt\_user"\].as<String>();

String password = doc\["mqtt\_password"\].as<String>();

String topic = doc\["mqtt\_topic\_info"\].as<String>();

Serial.println("Received MQTT config:");

Serial.print("Broker: ");

Serial.println(broker);

Serial.print("Port: ");

Serial.println(port);

Serial.print("Client ID: ");

Serial.println(id);

Serial.print("User: ");

Serial.println(user);

Serial.print("Topic: ");

Serial.println(topic);

// 验证输入

if (broker.length() == 0 || port <= 0 || port > 65535) {

webServer.send(400, "text/plain", "Error: Invalid broker or port");

return;

}

xSemaphoreTake(xMQTTMutex, portMAX\_DELAY);

mqtt\_topic = topic;

bool connectSuccess = mqttConnect(broker, port, id, user, password, 2000);

xSemaphoreGive(xMQTTMutex);

if (connectSuccess) {

String response = "Server:" + broker + "\\nPort:" + String(port) + "\\nTopic:" + topic;

webServer.send(200, "text/plain", response);

Serial.println("MQTT connected successfully");

} else {

webServer.send(500, "text/plain", "Error: Failed to connect to MQTT broker");

Serial.println("MQTT connection failed");

}

}

void handleBMPSize()

{

Serial.println("Handling BMP size request");

// 检查是否有请求体

if (!webServer.hasArg("plain")) {

webServer.send(400, "text/plain", "Bad Request: No JSON data");

return;

}

// 解析JSON数据

DynamicJsonDocument doc(256);

DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

if (error) {

Serial.print("JSON parse failed: ");

Serial.println(error.c\_str());

webServer.send(400, "text/plain", "Bad Request: Invalid JSON");

return;

}

bmp\_width = doc\["bmp\_width"\].as<int>();

bmp\_height = doc\["bmp\_height"\].as<int>();

Serial.print("BMP width: ");

Serial.println(bmp\_width);

Serial.print("BMP height: ");

Serial.println(bmp\_height);

webServer.send(200, "text/plain", "OK");

}

void handleBMP()

{

HTTPUpload& upload = webServer.upload();

if (upload.status == UPLOAD\_FILE\_START) {

Serial.print("BMP upload started. Filename: ");

Serial.println(upload.filename);

bmp\_data\_offset = 0;

} else if (upload.status == UPLOAD\_FILE\_WRITE) {

size\_t chunkSize = upload.currentSize;

if (bmp\_data\_offset + chunkSize > BMP\_BUFFER\_LIMIT) {

Serial.println("Error: BMP file too large");

return;

}

memcpy(bmp\_buffer + bmp\_data\_offset, upload.buf, chunkSize);

bmp\_data\_offset += chunkSize;

Serial.print("Received BMP chunk. Total size: ");

Serial.println(bmp\_data\_offset);

} else if (upload.status == UPLOAD\_FILE\_END) {

bmp\_data\_size = upload.totalSize;

Serial.print("BMP upload completed. Total size: ");

Serial.println(bmp\_data\_size);

if (bmp\_width > 0 && bmp\_height > 0) {

printer.printBMP(0, bmp\_width, bmp\_height, bmp\_buffer);

webServer.send(200, "text/plain", "BMP printed successfully");

} else {

webServer.send(400, "text/plain", "Error: BMP dimensions not set");

}

}

}

void handlePrint()

{

Serial.println("Handling print request");

// 获取打印类型

printType = urlDecode(webServer.arg("printType"));

Serial.print("Print type: ");

Serial.println(printType);

// 根据打印类型处理不同的数据

if (printType == "ASCII") {

Pdata = urlDecode(webServer.arg("Pdata"));

newLine = urlDecode(webServer.arg("newLine"));

Serial.print("ASCII data: ");

Serial.println(Pdata);

Serial.print("New line: ");

Serial.println(newLine);

printer.init();

printer.printASCII(Pdata);

} else if (printType == "QRCode") {

QRCode = urlDecode(webServer.arg("QRCode"));

newLine = urlDecode(webServer.arg("newLine"));

Serial.print("QRCode data: ");

Serial.println(QRCode);

printer.init();

printer.printQRCode(QRCode);

} else if (printType == "BarCode") {

BarCode = urlDecode(webServer.arg("BarCode"));

newLine = urlDecode(webServer.arg("newLine"));

Serial.print("Barcode data: ");

Serial.println(BarCode);

printer.init();

printer.setBarCodeHRI(HIDE);

printer.printBarCode(CODE128, BarCode);

} else {

webServer.send(400, "text/plain", "Error: Invalid print type");

return;

}

// 处理换行

if (newLine == "on") {

printer.newLine(1);

}

webServer.send(200, "text/plain", "OK");

Serial.println("Print request completed");

}

void webServerInit()

{

// 启动DNS服务器

dnsServer.start(DNS\_PORT, "\*", apIP);

// 设置路由处理器

webServer.onNotFound(handleRoot);

webServer.on("/", HTTP\_GET, handleRoot);

webServer.on("/print", HTTP\_GET, handlePrint);

webServer.on("/wifi\_config", HTTP\_POST, handleWiFiConfig);

webServer.on("/mqtt\_config", HTTP\_GET, handleMQTTConfig);

webServer.on("/device\_status", HTTP\_GET, handleStatusConfig);

webServer.on("/bmp\_size", HTTP\_POST, handleBMPSize);

webServer.on("/bmp", HTTP\_POST, \[\]() { webServer.send(200, "text/plain", "Ready for BMP upload"); }, handleBMP);

// 启动Web服务器

webServer.begin();

Serial.println("HTTP server started");

Serial.print("AP IP address: ");

Serial.println(WiFi.softAPIP());

}