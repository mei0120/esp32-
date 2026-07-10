#include "MailAlert.h"
#if __has_include("mail_config.h")
#include "mail_config.h"
#else
#include "mail_config.example.h"
#endif
#include "Task.h"
#include "net.h"
#include "LogUtil.h"

#if FIRE_EMAIL_ENABLE
#include <WiFiClientSecure.h>
#endif

static volatile bool emailPending = false;
static bool emailSending = false;
static bool lastEmailOk = false;
static unsigned long lastEmailSuccessMillis = 0;
static unsigned long nextEmailRetryMillis = 0;
static String lastEmailStatus = "disabled";
static const unsigned long FIRE_EMAIL_RETRY_MS = 60000UL;

bool fireEmailEnabled(){
  return FIRE_EMAIL_ENABLE;
}

bool fireEmailPending(){
  return emailPending;
}

bool fireEmailSending(){
  return emailSending;
}

bool lastFireEmailOk(){
  return lastEmailOk;
}

String fireEmailStatusText(){
  return lastEmailStatus;
}

void requestFireEmailAlert(bool force){
  if(!FIRE_EMAIL_ENABLE){
    lastEmailStatus = "disabled";
    return;
  }
  unsigned long now = millis();
  if(!force && lastEmailSuccessMillis != 0 && (now - lastEmailSuccessMillis) < FIRE_EMAIL_COOLDOWN_MS){
    lastEmailStatus = "cooldown";
    return;
  }
  emailPending = true;
  nextEmailRetryMillis = 0;
  lastEmailStatus = "pending";
}

#if FIRE_EMAIL_ENABLE
static const char b64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static String base64Encode(const String &input){
  String output;
  int value = 0;
  int valueBits = -6;
  for(size_t i = 0; i < input.length(); i++){
    value = (value << 8) + (uint8_t)input[i];
    valueBits += 8;
    while(valueBits >= 0){
      output += b64Table[(value >> valueBits) & 0x3F];
      valueBits -= 6;
    }
  }
  if(valueBits > -6){
    output += b64Table[((value << 8) >> (valueBits + 8)) & 0x3F];
  }
  while(output.length() % 4){
    output += '=';
  }
  return output;
}

static String readSmtpResponse(WiFiClientSecure &client, unsigned long timeoutMs = 5000){
  String response;
  unsigned long start = millis();
  while((millis() - start) < timeoutMs){
    while(client.available()){
      char c = (char)client.read();
      response += c;
      start = millis();
    }
    if(response.length() >= 4){
      int lastLine = response.lastIndexOf('\n');
      int lineStart = lastLine > 0 ? response.lastIndexOf('\n', lastLine - 1) + 1 : 0;
      if(lineStart >= 0 && response.length() >= (size_t)(lineStart + 4) && response[lineStart + 3] == ' '){
        break;
      }
    }
    delay(10);
  }
  response.trim();
  return response;
}

static bool smtpExpect(WiFiClientSecure &client, const String &command, const char *expected){
  if(command.length() > 0){
    client.print(command);
    client.print("\r\n");
  }
  String response = readSmtpResponse(client);
  if(!response.startsWith(expected)){
    lastEmailStatus = response.length() ? response : "smtp timeout";
    return false;
  }
  return true;
}

static bool sendFireEmailNow(){
  String body;
  body.reserve(240);
  body += "FIRE!\n";
  body += "Emergency smoke/fire alarm detected.\n";
  body += "Please check the device and room immediately.\n\n";
  body += "Time: ";
  body += currentFormattedTime();
  body += "\nIndoor temperature: ";
  body += temperature;
  body += " C\nHumidity: ";
  body += humidity;
  body += " %\nMQ2: ALARM\n";
  body += "Device: ESP32 Smart Calendar\n";

  WiFiClientSecure client;
  client.setInsecure();
  if(!client.connect(FIRE_SMTP_HOST, FIRE_SMTP_PORT)){
    lastEmailStatus = "connect failed";
    logInfo("SMTP connect failed: ");
    logInfo(FIRE_SMTP_HOST);
    logInfo(":");
    logInfoln(String(FIRE_SMTP_PORT));
    return false;
  }

  bool ok = smtpExpect(client, "", "220") &&
            smtpExpect(client, "EHLO esp32.local", "250") &&
            smtpExpect(client, "AUTH LOGIN", "334") &&
            smtpExpect(client, base64Encode(FIRE_AUTHOR_EMAIL), "334") &&
            smtpExpect(client, base64Encode(FIRE_AUTHOR_PASSWORD), "235") &&
            smtpExpect(client, String("MAIL FROM:<") + FIRE_AUTHOR_EMAIL + ">", "250") &&
            smtpExpect(client, String("RCPT TO:<") + FIRE_RECIPIENT_EMAIL + ">", "250") &&
            smtpExpect(client, "DATA", "354");

  if(ok){
    client.print("From: Smart Calendar <");
    client.print(FIRE_AUTHOR_EMAIL);
    client.print(">\r\nTo: Owner <");
    client.print(FIRE_RECIPIENT_EMAIL);
    client.print(">\r\nSubject: FIRE! FIRE ALARM FROM SMART CALENDAR\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n");
    client.print(body);
    client.print("\r\n.\r\n");
    String response = readSmtpResponse(client);
    ok = response.startsWith("250");
    if(!ok){
      lastEmailStatus = response.length() ? response : "smtp timeout";
    }
  }

  client.print("QUIT\r\n");
  client.stop();
  if(ok){
    lastEmailStatus = "sent";
  }
  return ok;
}
#endif

void processMailAlert(){
  if(!FIRE_EMAIL_ENABLE){
    return;
  }
  if(!emailPending || emailSending){
    return;
  }
  if(nextEmailRetryMillis != 0 && millis() < nextEmailRetryMillis){
    return;
  }
  if(!wifiConnected()){
    lastEmailStatus = "wait wifi";
    return;
  }

  emailPending = false;
  emailSending = true;
  lastEmailStatus = "sending";
  logInfoln("Sending fire email alert");

#if FIRE_EMAIL_ENABLE
  lastEmailOk = sendFireEmailNow();
#else
  lastEmailOk = false;
#endif
  if(lastEmailOk){
    lastEmailSuccessMillis = millis();
    nextEmailRetryMillis = 0;
  }else{
    emailPending = true;
    nextEmailRetryMillis = millis() + FIRE_EMAIL_RETRY_MS;
  }

  emailSending = false;
  logInfo("Fire email status: ");
  logInfoln(lastEmailStatus);
}
