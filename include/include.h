
#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include "log.h"
#include "config_opt.h"
#include "conn.h"
#include "mqtt.h"




enum err_t {
    ERR_CLIENT_CONFLICT = -10,
    ERR_CLIENT_IS_NEWER = -9,
    ERR_PUB_INVALID_CLIENT = -8,
    ERR_PUB_NO_ID_MAP = -7,
    ERR_PUB_NO_ID_USE = -6,
    ERR_PUB_INVALID_ID = -5,
    ERR_RPC_INVALID_RET = -4,
    ERR_RPC_FAILED = -3,
    ERR_SYS_FAILED = -2,
    ERR_WILD_POINTER = -1,

    SUCCESS = 0,
    ERR_PROTOCOL = 1,
    ERR_REJECT = 2,
    ERR_SYS_ERROR = 3,
    ERR_AUTH_FAILED = 4,
    ERR_NO_AUTH = 5,

    ERR_CONN_BREAK = 10,
    ERR_NOMEM = 11,
    ERR_INVAL = 12,
    ERR_NO_CONN = 13,
    ERR_CONN_REFUSED = 14,
    ERR_NOT_FOUND = 15,
    ERR_CONN_LOST = 16,
    ERR_TLS = 17,
    ERR_PAYLOAD_SIZE = 18,
    ERR_NOT_SUPPORTED = 19,
    ERR_ACL_DENIED = 20,
    ERR_UNKNOWN = 21,
    ERR_ERRNO = 22,
    ERR_EAI = 23,
};



#endif
