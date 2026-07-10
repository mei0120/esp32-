#pragma once

#ifndef FIRE_EMAIL_ENABLE
#define FIRE_EMAIL_ENABLE 0
#endif

#define FIRE_SMTP_HOST "smtp.qq.com"
#define FIRE_SMTP_PORT 465
#define FIRE_AUTHOR_EMAIL "your_email@qq.com"
#define FIRE_AUTHOR_PASSWORD "your_smtp_auth_code"
#define FIRE_RECIPIENT_EMAIL "target_email@qq.com"

#define FIRE_EMAIL_COOLDOWN_MS (10UL * 60UL * 1000UL)
