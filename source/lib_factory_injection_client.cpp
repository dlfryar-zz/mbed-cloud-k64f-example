//----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited or its affiliates.
//
// (C) COPYRIGHT 2016 ARM Limited or its affiliates.
// ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreement
// from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#include "mbed.h"
#include "EthernetInterface.h"

#include "lib_factory_injection_client.h"
#include "ftd_socket.h"
#include "spv_lib_iot.h"
#include "spv_lib_iot_test_only.h"
#include "identity_client.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "ijtl"

/**
 * The mBED device communicates port to the
 * Production Line Orchestrator
 */
#define FTD_SERVER_TCP_PORT 7777
FtdSocket *pServer = NULL;

EthernetInterface ethernet;
ProvisionStatus_t device_provision_status = DEVICE_PROVISION_FAILED;
NetworkInterface * network_handler = NULL;
volatile bool provisioned = false;
/**
 * Gets the mBED device IP address
 */
static const char *DeviceIpAddrGet(NetworkInterface * handler)
{
	int retries = 5;

	while (retries--) {
        const char *ip = handler->get_ip_address();
		if (ip != NULL)	{
			return ip;
		} else {
			wait(0.2);
		}
	}

	return NULL; /* failed to retrieve a valid IP address */
}

void provision_device()
{
    tr_info("Connecting to Ethernet interface...\r\n");
	
    const char *serverIpAddr = DeviceIpAddrGet(network_handler);
	if (serverIpAddr == NULL) {
        tr_debug("Failed retrieving device IP address!");
        device_provision_status = DEVICE_PROVISION_FAILED;
        provisioned = true;
        return;
	}
    printf("\r\nFactory Injection Server IP Address is %s:%d\r\n", serverIpAddr, FTD_SERVER_TCP_PORT);
	// Init SPV

//! [SpvInitialization]
	SaPvConfiguration_t spvConfig;

	SaPvStatus_t status = SaPvIoTInitConfigPrepare(&spvConfig);
	if (status != SA_PV_STATUS_OK) {
        tr_debug("SaPvIoTInitConfigPrepare failed");
        device_provision_status = DEVICE_PROVISION_FAILED;
        provisioned = true;
        return;
	}

	status = SaPvIoTInit(&spvConfig);
	if (status != SA_PV_STATUS_OK) {
        tr_debug("SaPvIoTInit failed");
        device_provision_status = DEVICE_PROVISION_FAILED;
        provisioned = true;
        return;
	}
//! [SpvInitialization]

    IdcStatus_t idc_status = IDC_STATUS_OK;

    idc_status = IdcInit();

    bool isDeviceConfigured = false;

    if(idc_status == IDC_STATUS_OK) {
        idc_status = IdcIsDeviceConfigured(&isDeviceConfigured);
        if(idc_status == IDC_STATUS_OK) {
            if(isDeviceConfigured) {
                device_provision_status = DEVICE_ALREADY_PROVISIONED;
                provisioned = true;
                return;
            }
        }
    }

	// Create the device server object
    pServer = new FtdSocket(FTD_SERVER_TCP_PORT);
//! [Initialization]
	if (pServer == NULL) {
        tr_debug("Failed instantiating FactoryInjectionClientSocket object");
        device_provision_status = DEVICE_PROVISION_FAILED;
        provisioned = true;
        return;
	}

    //Initializes Ethernet interface and prints its address
    bool result;
    result = pServer->InitNetworkInterface(network_handler, FTD_IPV4);
    if (result == false) {
        tr_debug("InitNetworkInterface Failed");
    }
//! [Initialization]

    printf("\r\nFactory Tool Device Client ready for Provisioning Tool...\n");
	// should never return
    bool success = pServer->Listen();

    //tr_debug("Factory tool work done with result %d", (int)success);
    // In case of a failure - disconnect and finalize SPV

    status = SaPvIoTFinalize();
	if (status != SA_PV_STATUS_OK) {
        tr_debug("SaPvIoTInit failed");
        device_provision_status = DEVICE_PROVISION_FAILED;
        provisioned = true;
        return;
	}

    device_provision_status = DEVICE_PROVISIONED;
    provisioned = true;
    pServer->Finish();
    return;
}
