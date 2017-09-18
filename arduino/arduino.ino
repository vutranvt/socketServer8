#include <String.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <SerialCommand.h>  // Thêm vào sketch thư viện Serial Command

// config device command
const char* server_arduino = "server_arduino";
const char* esp_arduino = "esp_arduino";

// config request command
#define chip_id_request "chip_id_request"
#define mac_address_request "mac_address_request"

String chipIdEsp = "";
String mac_address = "";

const byte RX = 3;          // Chân 3 được dùng làm chân RX
const byte TX = 2;          // Chân 2 được dùng làm chân TX

double ampAdc1 = 0;
double ampAdc2 = 0;
double ampAdc3 = 0;
int counter1 = 0;
int counter2 = 0;
int counter3 = 0;

unsigned int adc_zero = 512;

    
bool CONFIG_MAX_VALUE = false;

SoftwareSerial mySerial = SoftwareSerial(RX, TX);

SerialCommand sCmd(mySerial); // Khai báo biến sử dụng thư viện Serial Command

// int red = 4, blue = 5; // led đỏ đối vô digital 4, led xanh đối vô digital 5

void setup() {
    // Khởi tạo Serial ở baudrate 57600 để debug ở serial monitor
    Serial.begin(115200);
    // Khởi tạo Serial ở baudrate 57600 cho cổng Serial thứ hai, dùng cho việc kết nối với ESP8266
    mySerial.begin(115200);

    // Một số hàm trong thư viện Serial Command
    sCmd.addCommand(server_arduino, rxRequestServer);   //Khi có lệnh thì thực hiện hàm "configReadValue"
    sCmd.addCommand(esp_arduino, rxResponseEsp);  //Khi có lệnh thì thực hiện hàm "rxResponseEsp"

    delay(3000);
    txRequestEsp(mac_address_request);
}
    
void loop() {
    sCmd.readSerial();
    double data = adcRead(1000);
    txDataServer(data);
    //Bạn không cần phải thêm bất kỳ dòng code nào trong hàm loop này cả
}
/*
json_data = {
        "response" : "data"
        "data" : "..."
    }
*/
void txDataServer(double data){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["response"] = "data";
    root["data"] = data;

    mySerial.print(server_arduino);
    mySerial.print('\r');
    root.printTo(mySerial);
    mySerial.print('\r');  
}

void TaskSendData(void *pvParameters) {
    (void) pvParameters;

    double data1 = 0;
    double data2 = 0;
    double data3 = 0;

    data1 = ampAdc1;
    ampAdc1 = 0;
    counter1 = 0;

    data2 = ampAdc2;
    ampAdc2 = 0;
    counter2 = 0;

    data3 = ampAdc3;
    ampAdc3 = 0;
    counter3 = 0;

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["adc1"] = data1;
    root["adc2"] = data2;
    root["adc3"] = data3;

    root.printTo(mySerial);
    delay(4000);
}


/*
arduino-esp-request-response= {
    "request" : "request cmd"
    "response" : "request cmd"
    "data": "..."
}
*/
void txRequestEsp(String requestCmd){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["request"] = requestCmd;

    mySerial.print(esp_arduino);
    mySerial.print('\r');
    root.printTo(mySerial);
    mySerial.print('\r');    
}
void rxResponseEsp(){
    Serial.println(esp_arduino);
    char *json = sCmd.next(); //Chỉ cần một dòng này để đọc tham số nhận đươc
    Serial.println(json);
    StaticJsonBuffer<200> jsonBuffer; //tạo Buffer json có khả năng chứa tối đa 200 ký tự
    JsonObject& root = jsonBuffer.parseObject(json);//đặt một biến root mang kiểu json

    String response = root["response"];
    String data = root["data"];
    if(response==chip_id_request){
        chipIdEsp = data;
    }
    if(response==mac_address_request){
        mac_address = data;
    }
}
/*
arduino-server-request-response = {
    "request" : "request cmd",
    "response" : "request cmd",
    "data" : "...."
}
*/
void rxRequestServer(String deviceCmd){
    Serial.println(server_arduino);
    char *json = sCmd.next(); //Chỉ cần một dòng này để đọc tham số nhận đươc
    Serial.println(json);
    StaticJsonBuffer<200> jsonBuffer; //tạo Buffer json có khả năng chứa tối đa 200 ký tự
    JsonObject& root = jsonBuffer.parseObject(json);//đặt một biến root mang kiểu json

    String requestCmd = root["request"];

    if(requestCmd==chip_id_request){
        txResponse(deviceCmd, requestCmd, chipIdEsp);
    }
    if(requestCmd==mac_address_request){
        txResponse(deviceCmd, requestCmd, mac_address);
    }
}
void txResponse(String deviceCmd, String requestCmd, String dataResponse){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["response"] = requestCmd;
    root["data"] = dataResponse;

    mySerial.print(deviceCmd);
    mySerial.print('\r');
    root.printTo(mySerial);
    mySerial.print('\r'); 
}

double adcRead(long sampleTime) {

    double result = 0;
    double temp = 0;
    double average = 0;

    unsigned long counter = 0;

    long analogValue;              //value read from the sensor

    unsigned long startTime = millis();

    while ((unsigned long)(millis() - startTime) < sampleTime) //sample for 1 Sec
    {
        analogValue = analogRead(A0) - adc_zero;
        Serial.println(analogValue);
        average = average + (double)(analogValue * analogValue);
        counter++;
    }
//    temp = sqrt(average / (double)counter);
//    Serial.println(temp);

    result = sqrt(average / (double)counter) * (22 / 1024.0); // Lay gia tri trung binh

    result = result - 0.10;

    if (result < 0.20)
    {
        result = 0;
    } else {
        result = result + 0.10;
    }
    result = result * 10;   // Hệ số TI

    Serial.println(result);
    return result;
}
