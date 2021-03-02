#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* pass = "YOUR_WIFI_PASS";

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n" \
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n" \
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n" \
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n" \
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n" \
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n" \
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n" \
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n" \
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n" \
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n" \
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n" \
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n" \
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n" \
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n" \
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n" \
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n" \
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n" \
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n" \
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n" \
"-----END CERTIFICATE-----\n";

#define PRICE_LOG_SIZE 1200
float price_log[PRICE_LOG_SIZE];
unsigned int price_log_head = 0;
unsigned int price_log_count = 0;

#define UP_TRI 0x9A
#define DOWN_TRI 0x98
#define UP_ARROW 0xFC
#define DOWN_ARROW 0xFA

void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8E1, 25, 27);
  delay(1000);
  Serial.print("Connecting to WiFi...");
  Serial2.write(0x0E);
  Serial2.write(0x0D);
  Serial2.write(0x19);
  Serial2.print("Connecting...");
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial2.print(".");
  }
  Serial.println("");

  Serial.println("Connected.");

  
}

char lastDir = ' ';
float lastPrice = 0;

void loop() {
  if((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin("https://api.coinbase.com/v2/prices/BTC-USD/spot", root_ca);
    digitalWrite(2, HIGH);
    int statusCode = http.GET();
    digitalWrite(2, LOW);
    if(statusCode > 0) {
      String result = http.getString().substring(49,56);
      float price = result.toFloat();
      float diff = price - lastPrice;
      if(lastPrice != 0 && (abs(diff) > 0.01)) {
        if(diff > 0) {
          lastDir = UP_TRI;
        } else if(diff < 0) {
          lastDir = DOWN_TRI;
        }
      }
      lastPrice = price;

      Serial.println(result);
      Serial2.println("");
      Serial2.print(result);
      Serial2.print(lastDir);

      int nextIndex = (price_log_head + 1) % PRICE_LOG_SIZE;
      if(price_log_count == PRICE_LOG_SIZE) {
        float hourAgo = price_log[nextIndex];
        diff = 100 * (price - hourAgo) / hourAgo;
      } else {
        diff = 100 * (price - price_log[0]) / price_log[0];
        price_log_count++;
      }
        
        char dir = (diff > 0) ? UP_TRI : DOWN_TRI;
        if(price_log_count < PRICE_LOG_SIZE) {
          dir += (DOWN_ARROW - DOWN_TRI);
        }
        String percentOut = String(String(abs(diff),2) + "%");
        Serial2.write(0x1B);
        Serial2.write(0x48);
        Serial2.write((char) 19 - percentOut.length());
        Serial2.write(dir);
        Serial2.print(percentOut);

      price_log[price_log_head] = price;
      price_log_head = nextIndex;
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }
  delay(3000);
}
