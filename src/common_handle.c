#include "include.h"
#include "common_handle.h"
#include "common_process.h"

int common_disconnect_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    int rc = SUCCESS;
    DEBUG(10, "disconnect handle\n");

    rc = common_disconnect_process(conn);

    return rc;
}

int common_pingreq_handle(struct conn_context *conn, struct mqtt_packet *packet)
{
    DEBUG(10, "pingreq handle\n");
    return common_ping_process(conn);
}

