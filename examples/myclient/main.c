#include "liblwm2m.h"
#include "object_security.c"
#include "object_server.c"
#include "object_device.c"

int main()
{
    lwm2m_context_t * context;
    lwm2m_object_t * objArray[3];
    int result;
    time_t timeout;

    context = lwm2m_init(NULL);
    if (!context) return -1;

    objArray[0] = get_security_object(); // Bootstrap-Server vorkonfiguriert
    objArray[1] = get_server_object();   // wird durch Bootstrap überschrieben
    objArray[2] = get_object_device();   // statische Geräteinfos

    lwm2m_configure(context, "clipperIV", NULL, NULL, 3, objArray);

    while (1) {
        result = lwm2m_step(context, &timeout);
        if (result != 0) break;
    }

    lwm2m_close(context);
    return 0;
}

