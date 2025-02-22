//Before setup

// Webserver constants
// const char* SSID = "Kekatpure_home";
// const char* PASSWD = "628OldStoneWifi";
// ESP8266WebServer server(80);

// void serveIndex() {
//   if (! LittleFS.begin()) {
//     Serial.println("Error mounting LittleFS");    
//   }
//   server.serveStatic("/", LittleFS, "/index.html");  
// }

// void handlexy() {
//   JSONVar coords = JSON.parse(server.arg(0));
  
//   if (JSON.typeof(coords) == "undefined"){
//     Serial.println(server.arg(0));
//     Serial.println("Failed to parse request payload");
//     return;
//   }

//   // Received from the front end
//   double x = coords["x"];
//   double y = coords["y"];

//   // Derived parameters
//   double r = sqrt(x * x + y * y);
//   double theta_rad = PI / 2.0 - atan2(y, x);
//   double theta_deg = theta_rad * RAD_TO_DEG;

//   // Conversion to motor PWMs
//   double base_speed = r; // speed is distance from the center
//   double left_speed = base_speed;
//   double right_speed = base_speed;
//   double factor = 1.0 - fabs(sin(theta_rad));

//   if ((theta_deg >= 0.0) && (theta_deg <= 90.0)) {
//     right_speed *= factor;    
//   } else if ((theta_deg < 0.0) && (theta_deg >= -90.0)) {
//     left_speed *= factor;
//   }

//   // Convert speeds to PWM
//   left_speed *= PWM;
//   right_speed *= PWM; 

//   // Change speed state
//   GLOBAL_LEFT_SPEED = left_speed;
//   GLOBAL_RIGHT_SPEED = right_speed;  

//   String fmt = "LEFT_SPEED = %0.2f, RIGHT_SPEED = %0.2f\n";
//   Serial.printf(fmt.c_str(), GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);

//   // Write to Drive pins
//   setSpeed(GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);
// }

// setup code
  // Connect Wifi
  // Serial.printf("Connecting to: %s\n", SSID);
  // WiFi.begin(SSID, PASSWD);  
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  // Serial.println("");
  // Serial.println("Connected");
  // Serial.println(WiFi.localIP()); 

  // Start Webserver
  // serveIndex();
  // server.on("/xy", handlexy);
  // server.begin();  