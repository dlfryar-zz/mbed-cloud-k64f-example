#ifndef LIB_FACTORY_INJECTION_CLIENT_H
#define LIB_FACTORY_INJECTION_CLIENT_H

#include "NetworkInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEVICE_PROVISIONED,
    DEVICE_ALREADY_PROVISIONED,
    DEVICE_PROVISION_FAILED

} ProvisionStatus_t;

extern ProvisionStatus_t device_provision_status;
extern NetworkInterface * network_handler;
extern volatile bool provisioned;
void provision_device();

#ifdef __cplusplus
}
#endif


#endif // LIB_FACTORY_INJECTION_CLIENT_H
