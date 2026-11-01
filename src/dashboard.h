#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "vehicle.h"

void dashboard_splash(void);
void dashboard_render(const Vehicle *v);
void dashboard_summary(const Vehicle *v);

#endif
