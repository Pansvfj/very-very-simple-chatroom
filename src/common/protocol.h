#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "structs.h"

extern char* pack_msg(msg *m);

extern MSG_ERROR analysis_msg(const char *data, msg *m);

extern user_info g_current_user; //only client use

#endif // PROTOCOL_H
