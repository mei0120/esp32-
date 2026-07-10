#ifndef __MAIL_ALERT_H
#define __MAIL_ALERT_H

#include <Arduino.h>

void requestFireEmailAlert(bool force = false);
void processMailAlert();
bool fireEmailEnabled();
bool fireEmailPending();
bool fireEmailSending();
bool lastFireEmailOk();
String fireEmailStatusText();

#endif
