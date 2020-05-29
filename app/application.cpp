#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>
#include <Libraries/OneWire/OneWire.h>

#include <Libraries/DS18S20/ds18s20.h>
#include <Libraries/RCSwitch/RCSwitch.h>

DS18S20 ReadTemp;
Timer procTimer;
HttpServer server;
FTPServer ftp;

BssList networks;
String network, password;
Timer connectionTimer;
Timer receiveTimer;
Timer sendTimer;
RCSwitch mySwitch = RCSwitch();
float temp;
const int gpio_ports[] = {5, 12, 13, 14, 16};

void sendRF()
{
	mySwitch.send(5393, 24);
	Serial.println("RF command successful sent");
}

void receiveRF()
{
	if (mySwitch.available())
	{
		if (mySwitch.getReceivedValue() == 0)
		{
			Serial.print("Unknown encoding");
		} else {
			Serial.print("Received ");
			Serial.print( mySwitch.getReceivedValue() );
			Serial.print(" / ");
			Serial.print( mySwitch.getReceivedBitlength() );
			Serial.print("bit ");
			Serial.print("Protocol: ");
			Serial.println( mySwitch.getReceivedProtocol() );
		}

		mySwitch.resetAvailable();
	}
}


void readData()
{

	if (!ReadTemp.MeasureStatus())  // the last measurement completed
	{
		ReadTemp.StartMeasure();  // next measure, result after 1.2 seconds * number of sensors
	}
	else
		Serial.println("No valid Measure so far! wait please");


}


void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	response.sendTemplate(tmpl); // will be automatically deleted
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
		AppSettings.ip = request.getPostParameter("ip");
		AppSettings.netmask = request.getPostParameter("netmask");
		AppSettings.gateway = request.getPostParameter("gateway");
        
        String backend_ssid = request.getPostParameter("backendssid");
        String backend_password = request.getPostParameter("password1");
        
        if (!backend_ssid.equals("") && !backend_ssid.equals(AppSettings.backendssid)) {
            AppSettings.backendssid = backend_ssid;
        }
        
        if (!backend_password.equals("******") && !backend_password.equals("") && !backend_password.equals(AppSettings.backendpassword)) {
            AppSettings.backendpassword = backend_password;
        }
        
		debugf("Updating IP settings: %d", AppSettings.ip.isNull());
		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
	auto &vars = tmpl->variables();

	bool dhcp = WifiStation.isEnabledDHCP();
	vars["dhcpon"] = dhcp ? "checked='checked'" : "";
	vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";

	if (!WifiStation.getIP().isNull())
	{
		vars["ip"] = WifiStation.getIP().toString();
		vars["netmask"] = WifiStation.getNetworkMask().toString();
		vars["gateway"] = WifiStation.getNetworkGateway().toString();
	}
	else
	{
		vars["ip"] = "192.168.1.77";
		vars["netmask"] = "255.255.255.0";
		vars["gateway"] = "192.168.1.1";
	}
    
    vars["backendssid"] = AppSettings.backendssid;

	response.sendTemplate(tmpl); // will be automatically deleted
}

void onSensors(HttpRequest &request, HttpResponse &response)
{

	TemplateFileStream *tmpl = new TemplateFileStream("sensors.html");
	auto &vars = tmpl->variables();
	
	//vars["temperature"] = temp;
	response.sendTemplate(tmpl); // will be automatically deleted
}

void onAjaxSensors(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();
	uint8_t a;
	uint64_t info;
	
	int gpio = -1;
	int val = -1;
	
	if (request.getPostParameter("gpio").length() > 0) {
	    gpio = request.getPostParameter("gpio").toInt();
	} else if (request.getQueryParameter("gpio").length() > 0) {
	    gpio = request.getQueryParameter("gpio").toInt();
	}
	
	if (request.getPostParameter("set_value").length() > 0) {
	    val = request.getPostParameter("set_value").toInt();
	} else if (request.getQueryParameter("set_value").length() > 0) {
	    val = request.getQueryParameter("set_value").toInt();
	}
	
	if (gpio >= 0 && val >= 0) {
	  bool can_use = false;
	  
	  switch(gpio) {
	    case 4:
	    case 5:
	    case 12:
	    case 13:
	    case 14:
	    case 16:
	      can_use = true;
	      break;
	    default:
	      break;
	  }
	  
	  if (can_use) {
	    pinMode(gpio, OUTPUT);
	    if (val > 0) {
		digitalWrite(gpio, 1);
	    } else {
		digitalWrite(gpio, 0);
	    }
	    
	    delay(1000); // to be ready for digitalRead(gpio); WARNING possible destroy pin on ESP8266, checked with arduino only
	    
	  }
	  

	}

	JsonArray& sensorlist = json.createNestedArray("sensors");
	JsonArray& switchlist = json.createNestedArray("switches");
	
	for (int i = 0; i < ReadTemp.GetSensorsCount(); i++)
	{
		JsonObject &item = sensorlist.createNestedObject(); //need to get index somehow to update previous data in memory

		item["temperature"] = (float)ReadTemp.GetCelsius(i);

		
		if (ReadTemp.IsValidTemperature(i))
		  item["valid"] = true;
		else
		  item["valid"] = false;
		
		
	      info=ReadTemp.GetSensorID(i)>>32;
	      Serial.print((uint32_t)info,16);
	      Serial.print((uint32_t)ReadTemp.GetSensorID(i),16);
	      String addr_high =  String((uint32_t)info, HEX);
	      String addr_low =  String((uint32_t)ReadTemp.GetSensorID(i),HEX);
	      
	      
	      item["id"] = String(addr_high + addr_low);

	}
	
	for (int i = 0; i < sizeof(gpio_ports) / sizeof(int); i++) {
	  JsonObject &item = switchlist.createNestedObject();
	  
	  int val = digitalRead(gpio_ports[i]);

	  item["id"] = String(String("gpio") + String((int)gpio_ports[i]));
	  item["value"] = (int)val;
	  Serial.print("val=");
	  Serial.println(val);

	}
	
	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}


void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void onAjaxNetworkList(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["status"] = (bool)true;

	bool connected = WifiStation.isConnected();
	json["connected"] = connected;
	if (connected)
	{
		// Copy full string to JSON buffer memory
		json["network"]= WifiStation.getSSID();
	}

	JsonArray& netlist = json.createNestedArray("available");
	for (int i = 0; i < networks.count(); i++)
	{
		if (networks[i].hidden) continue;
		JsonObject &item = netlist.createNestedObject();
		item["id"] = (int)networks[i].getHashId();
		// Copy full string to JSON buffer memory
		item["title"] = networks[i].ssid;
		item["signal"] = networks[i].rssi;
		item["encryption"] = networks[i].getAuthorizationMethodName();
	}

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void makeConnection()
{
	WifiStation.enable(true);
	WifiStation.config(network, password);

	AppSettings.ssid = network;
	AppSettings.password = password;
	AppSettings.save();

	network = ""; // task completed
}

void onAjaxConnect(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String curNet = request.getPostParameter("network");
	String curPass = request.getPostParameter("password");

	bool updating = curNet.length() > 0 && (WifiStation.getSSID() != curNet || WifiStation.getPassword() != curPass);
	bool connectingNow = WifiStation.getConnectionStatus() == eSCS_Connecting || network.length() > 0;

	if (updating && connectingNow)
	{
		debugf("wrong action: %s %s, (updating: %d, connectingNow: %d)", network.c_str(), password.c_str(), updating, connectingNow);
		json["status"] = (bool)false;
		json["connected"] = (bool)false;
	}
	else
	{
		json["status"] = (bool)true;
		if (updating)
		{
			network = curNet;
			password = curPass;
			debugf("CONNECT TO: %s %s", network.c_str(), password.c_str());
			json["connected"] = false;
			connectionTimer.initializeMs(1200, makeConnection).startOnce();
		}
		else
		{
			json["connected"] = WifiStation.isConnected();
			debugf("Network already selected. Current status: %s", WifiStation.getConnectionStatusName());
		}
	}

	if (!updating && !connectingNow && WifiStation.isConnectionFailed())
		json["error"] = WifiStation.getConnectionStatusName();

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/ipconfig", onIpConfig);
	server.addPath("/sensors", onSensors);
	server.addPath("/ajax/sensors", onAjaxSensors);
	server.addPath("/ajax/get-networks", onAjaxNetworkList);
	server.addPath("/ajax/connect", onAjaxConnect);
	server.setDefaultHandler(onFile);
}

void startFTP()
{
	if (!fileExist("index.html"))
		fileSetContent("index.html", "<h3>Please connect to FTP and upload files from folder 'web/build' (details in code)</h3>");

	// Start FTP server
	ftp.listen(21);
	ftp.addUser(AppSettings.backendssid.c_str(), AppSettings.backendpassword.c_str()); // FTP account
}

// Will be called when system initialization was completed
void startServers()
{
	startFTP();
	startWebServer();
}

void networkScanCompleted(bool succeeded, BssList list)
{
	if (succeeded)
	{
		for (int i = 0; i < list.count(); i++)
			if (!list[i].hidden && list[i].ssid.length() > 0)
				networks.add(list[i]);
	}
	networks.sort([](const BssInfo& a, const BssInfo& b){ return b.rssi - a.rssi; } );
}

void init()
{
	spiffs_mount(); // Mount file system, in order to work with files

	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Enable debug output to serial
	
	ReadTemp.Init(4);  			// select PIN It's required for one-wire initialization!
	ReadTemp.StartMeasure(); // first measure start,result after 1.2 seconds * number of sensors
	//mySwitch.enableTransmit(5); // pin GPIO5 - transmit
    	mySwitch.enableReceive(16); // pin GPIO4  - receive        
    	/* Init gpio values (inverse logic */
        
    	for (int i = 0; i < sizeof(gpio_ports) / sizeof(int); i++) {
        	pinMode(gpio_ports[i], OUTPUT);
        	digitalWrite(gpio_ports[i], 1);
    	}
        
        
	procTimer.initializeMs(10000, readData).start();   // every 10 seconds
	
	AppSettings.load();

	WifiStation.enable(true);

	if (AppSettings.exist())
	{
		WifiStation.config(AppSettings.ssid, AppSettings.password);
		if (!AppSettings.dhcp && !AppSettings.ip.isNull())
			WifiStation.setIP(AppSettings.ip, AppSettings.netmask, AppSettings.gateway);
        if (AppSettings.backendssid.equals("") || AppSettings.backendpassword.equals("")) {
        

            String mac_id = WifiStation.getMAC();
            mac_id.replace(":", "");
            mac_id = mac_id.substring(6, mac_id.length() - 1);
            mac_id.toUpperCase();
            AppSettings.backendssid = String(String(WIFI_SSID) + String('_') + mac_id);
            AppSettings.backendpassword = String(WIFI_PWD);
            AppSettings.save();
        }
        
            
	}

   	//sendTimer.initializeMs(1000, sendRF).start();
    	receiveTimer.initializeMs(20, receiveRF).start();


	WifiStation.startScan(networkScanCompleted);

	// Start AP for configuration
	WifiAccessPoint.enable(true);
	WifiAccessPoint.config(AppSettings.backendssid, AppSettings.backendpassword, APP_SETTINGS_CFG_AUTH);

	// Run WEB server on system ready
	System.onReady(startServers);
}
