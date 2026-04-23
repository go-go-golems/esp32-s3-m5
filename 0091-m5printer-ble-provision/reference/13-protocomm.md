## Protocol Communication

[\[中文\]](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/provisioning/protocomm.html)

## Overview

The Protocol Communication (protocomm) component manages secure sessions and provides the framework for multiple transports. The application can also use the protocomm layer directly to have application-specific extensions for the provisioning or non-provisioning use cases.

Following features are available for provisioning:

> - Communication security at the application level
> 	> - `protocomm_security0` (no security)
> 	> - `protocomm_security1` (Curve25519 key exchange + AES-CTR encryption/decryption)
> 	> - `protocomm_security2` (SRP6a-based key exchange + AES-GCM encryption/decryption)
> - Proof-of-possession (support with protocomm\_security1 only)
> - Salt and Verifier (support with protocomm\_security2 only)

Protocomm internally uses protobuf (protocol buffers) for secure session establishment. Users can choose to implement their own security (even without using protobuf). Protocomm can also be used without any security layer.

Protocomm provides the framework for various transports:

- Bluetooth LE
- Wi-Fi (SoftAP + HTTPD)
- Console, in which case the handler invocation is automatically taken care of on the device side. See Transport Examples below for code snippets.

Note that for protocomm\_security1 and protocomm\_security2, the client still needs to establish sessions by performing the two-way handshake.

See [Unified Provisioning](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/provisioning.html) for more details about the secure handshake logic.

## Enabling Protocomm Security Version

The protocomm component provides a project configuration menu to enable/disable support of respective security versions. The respective configuration options are as follows:

> - Support `protocomm_security0`, with no security: [CONFIG\_ESP\_PROTOCOMM\_SUPPORT\_SECURITY\_VERSION\_0](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig-reference.html#config-esp-protocomm-support-security-version-0), this option is enabled by default.
> - Support `protocomm_security1` with Curve25519 key exchange + AES-CTR encryption/decryption: [CONFIG\_ESP\_PROTOCOMM\_SUPPORT\_SECURITY\_VERSION\_1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig-reference.html#config-esp-protocomm-support-security-version-1), this option is enabled by default.
> - Support `protocomm_security2` with SRP6a-based key exchange + AES-GCM encryption/decryption: [CONFIG\_ESP\_PROTOCOMM\_SUPPORT\_SECURITY\_VERSION\_2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig-reference.html#config-esp-protocomm-support-security-version-2).

Note

Enabling multiple security versions at once offers the ability to control them dynamically but also increases the firmware size.

Warning

`protocomm_security0` provides no encryption or authentication and should not be used in production. `protocomm_security2` (SRP6a + AES-GCM) is the recommended security version for all production use cases.

## SoftAP + HTTP Transport Example with Security 2

For sample usage, see [network\_provisioning/src/scheme\_softap.c](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/scheme_softap.c).

```c
/* The endpoint handler to be registered with protocomm. This simply echoes back the received data. */
esp_err_t echo_req_handler (uint32_t session_id,
                            const uint8_t *inbuf, ssize_t inlen,
                            uint8_t **outbuf, ssize_t *outlen,
                            void *priv_data)
{
    /* Session ID may be used for persistence. */
    printf("Session ID : %d", session_id);

    /* Echo back the received data. */
    *outlen = inlen;            /* Output the data length updated. */
    *outbuf = malloc(inlen);    /* This is to be deallocated outside. */
    memcpy(*outbuf, inbuf, inlen);

    /* Private data that was passed at the time of endpoint creation. */
    uint32_t *priv = (uint32_t *) priv_data;
    if (priv) {
        printf("Private data : %d", *priv);
    }

    return ESP_OK;
}

static const char sec2_salt[] = {0xf7, 0x5f, 0xe2, 0xbe, 0xba, 0x7c, 0x81, 0xcd};
static const char sec2_verifier[] = {0xbf, 0x86, 0xce, 0x63, 0x8a, 0xbb, 0x7e, 0x2f, 0x38, 0xa8, 0x19, 0x1b, 0x35,
    0xc9, 0xe3, 0xbe, 0xc3, 0x2b, 0x45, 0xee, 0x10, 0x74, 0x22, 0x1a, 0x95, 0xbe, 0x62, 0xf7, 0x0c, 0x65, 0x83, 0x50,
    0x08, 0xef, 0xaf, 0xa5, 0x94, 0x4b, 0xcb, 0xe1, 0xce, 0x59, 0x2a, 0xe8, 0x7b, 0x27, 0xc8, 0x72, 0x26, 0x71, 0xde,
    0xb2, 0xf2, 0x80, 0x02, 0xdd, 0x11, 0xf0, 0x38, 0x0e, 0x95, 0x25, 0x00, 0xcf, 0xb3, 0x3f, 0xf0, 0x73, 0x2a, 0x25,
    0x03, 0xe8, 0x51, 0x72, 0xef, 0x6d, 0x3e, 0x14, 0xb9, 0x2e, 0x9f, 0x2a, 0x90, 0x9e, 0x26, 0xb6, 0x3e, 0xc7, 0xe4,
    0x9f, 0xe3, 0x20, 0xce, 0x28, 0x7c, 0xbf, 0x89, 0x50, 0xc9, 0xb6, 0xec, 0xdd, 0x81, 0x18, 0xf1, 0x1a, 0xd9, 0x7a,
    0x21, 0x99, 0xf1, 0xee, 0x71, 0x2f, 0xcc, 0x93, 0x16, 0x34, 0x0c, 0x79, 0x46, 0x23, 0xe4, 0x32, 0xec, 0x2d, 0x9e,
    0x18, 0xa6, 0xb9, 0xbb, 0x0a, 0xcf, 0xc4, 0xa8, 0x32, 0xc0, 0x1c, 0x32, 0xa3, 0x97, 0x66, 0xf8, 0x30, 0xb2, 0xda,
    0xf9, 0x8d, 0xc3, 0x72, 0x72, 0x5f, 0xe5, 0xee, 0xc3, 0x5c, 0x24, 0xc8, 0xdd, 0x54, 0x49, 0xfc, 0x12, 0x91, 0x81,
    0x9c, 0xc3, 0xac, 0x64, 0x5e, 0xd6, 0x41, 0x88, 0x2f, 0x23, 0x66, 0xc8, 0xac, 0xb0, 0x35, 0x0b, 0xf6, 0x9c, 0x88,
    0x6f, 0xac, 0xe1, 0xf4, 0xca, 0xc9, 0x07, 0x04, 0x11, 0xda, 0x90, 0x42, 0xa9, 0xf1, 0x97, 0x3d, 0x94, 0x65, 0xe4,
    0xfb, 0x52, 0x22, 0x3b, 0x7a, 0x7b, 0x9e, 0xe9, 0xee, 0x1c, 0x44, 0xd0, 0x73, 0x72, 0x2a, 0xca, 0x85, 0x19, 0x4a,
    0x60, 0xce, 0x0a, 0xc8, 0x7d, 0x57, 0xa4, 0xf8, 0x77, 0x22, 0xc1, 0xa5, 0xfa, 0xfb, 0x7b, 0x91, 0x3b, 0xfe, 0x87,
    0x5f, 0xfe, 0x05, 0xd2, 0xd6, 0xd3, 0x74, 0xe5, 0x2e, 0x68, 0x79, 0x34, 0x70, 0x40, 0x12, 0xa8, 0xe1, 0xb4, 0x6c,
    0xaa, 0x46, 0x73, 0xcd, 0x8d, 0x17, 0x72, 0x67, 0x32, 0x42, 0xdc, 0x10, 0xd3, 0x71, 0x7e, 0x8b, 0x00, 0x46, 0x9b,
    0x0a, 0xe9, 0xb4, 0x0f, 0xeb, 0x70, 0x52, 0xdd, 0x0a, 0x1c, 0x7e, 0x2e, 0xb0, 0x61, 0xa6, 0xe1, 0xa3, 0x34, 0x4b,
    0x2a, 0x3c, 0xc4, 0x5d, 0x42, 0x05, 0x58, 0x25, 0xd3, 0xca, 0x96, 0x5c, 0xb9, 0x52, 0xf9, 0xe9, 0x80, 0x75, 0x3d,
    0xc8, 0x9f, 0xc7, 0xb2, 0xaa, 0x95, 0x2e, 0x76, 0xb3, 0xe1, 0x48, 0xc1, 0x0a, 0xa1, 0x0a, 0xe8, 0xaf, 0x41, 0x28,
    0xd2, 0x16, 0xe1, 0xa6, 0xd0, 0x73, 0x51, 0x73, 0x79, 0x98, 0xd9, 0xb9, 0x00, 0x50, 0xa2, 0x4d, 0x99, 0x18, 0x90,
    0x70, 0x27, 0xe7, 0x8d, 0x56, 0x45, 0x34, 0x1f, 0xb9, 0x30, 0xda, 0xec, 0x4a, 0x08, 0x27, 0x9f, 0xfa, 0x59, 0x2e,
    0x36, 0x77, 0x00, 0xe2, 0xb6, 0xeb, 0xd1, 0x56, 0x50, 0x8e};

/* The example function for launching a protocomm instance over HTTP. */
protocomm_t *start_pc()
{
    protocomm_t *pc = protocomm_new();

    /* Config for protocomm_httpd_start(). */
    protocomm_httpd_config_t pc_config = {
        .data = {
        .config = PROTOCOMM_HTTPD_DEFAULT_CONFIG()
        }
    };

    /* Start the protocomm server on top of HTTP. */
    protocomm_httpd_start(pc, &pc_config);

    /* Create Security2 params object from salt and verifier. It must be valid throughout the scope of protocomm endpoint. This does not need to be static, i.e., could be dynamically allocated and freed at the time of endpoint removal. */
    const static protocomm_security2_params_t sec2_params = {
        .salt = (const uint8_t *) salt,
        .salt_len = sizeof(salt),
        .verifier = (const uint8_t *) verifier,
        .verifier_len = sizeof(verifier),
    };

    /* Set security for communication at the application level. Just like for request handlers, setting security creates an endpoint and registers the handler provided by protocomm_security1. One can similarly use protocomm_security0. Only one type of security can be set for a protocomm instance at a time. */
    protocomm_set_security(pc, "security_endpoint", &protocomm_security2, &sec2_params);

    /* Private data passed to the endpoint must be valid throughout the scope of protocomm endpoint. This need not be static, i.e., could be dynamically allocated and freed at the time of endpoint removal. */
    static uint32_t priv_data = 1234;

    /* Add a new endpoint for the protocomm instance, identified by a unique name, and register a handler function along with the private data to be passed at the time of handler execution. Multiple endpoints can be added as long as they are identified by unique names. */
    protocomm_add_endpoint(pc, "echo_req_endpoint",
                           echo_req_handler, (void *) &priv_data);
    return pc;
}

/* The example function for stopping a protocomm instance. */
void stop_pc(protocomm_t *pc)
{
    /* Remove the endpoint identified by its unique name. */
    protocomm_remove_endpoint(pc, "echo_req_endpoint");

    /* Remove the security endpoint identified by its name. */
    protocomm_unset_security(pc, "security_endpoint");

    /* Stop the HTTP server. */
    protocomm_httpd_stop(pc);

    /* Delete, namely deallocate the protocomm instance. */
    protocomm_delete(pc);
}
```

## SoftAP + HTTP Transport Example with Security 1

For sample usage, see [network\_provisioning/src/scheme\_softap.c](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/scheme_softap.c).

```c
/* The endpoint handler to be registered with protocomm. This simply echoes back the received data. */
esp_err_t echo_req_handler (uint32_t session_id,
                            const uint8_t *inbuf, ssize_t inlen,
                            uint8_t **outbuf, ssize_t *outlen,
                            void *priv_data)
{
    /* Session ID may be used for persistence. */
    printf("Session ID : %d", session_id);

    /* Echo back the received data. */
    *outlen = inlen;            /* Output the data length updated. */
    *outbuf = malloc(inlen);    /* This is to be deallocated outside. */
    memcpy(*outbuf, inbuf, inlen);

    /* Private data that was passed at the time of endpoint creation. */
    uint32_t *priv = (uint32_t *) priv_data;
    if (priv) {
        printf("Private data : %d", *priv);
    }

    return ESP_OK;
}

/* The example function for launching a protocomm instance over HTTP. */
protocomm_t *start_pc(const char *pop_string)
{
    protocomm_t *pc = protocomm_new();

    /* Config for protocomm_httpd_start(). */
    protocomm_httpd_config_t pc_config = {
        .data = {
        .config = PROTOCOMM_HTTPD_DEFAULT_CONFIG()
        }
    };

    /* Start the protocomm server on top of HTTP. */
    protocomm_httpd_start(pc, &pc_config);

    /* Create security1 params object from pop_string. It must be valid throughout the scope of protocomm endpoint. This need not be static, i.e., could be dynamically allocated and freed at the time of endpoint removal. */
    const static protocomm_security1_params_t sec1_params = {
        .data = (const uint8_t *) strdup(pop_string),
        .len = strlen(pop_string)
    };

    /* Set security for communication at the application level. Just like for request handlers, setting security creates an endpoint and registers the handler provided by protocomm_security1. One can similarly use protocomm_security0. Only one type of security can be set for a protocomm instance at a time. */
    protocomm_set_security(pc, "security_endpoint", &protocomm_security1, &sec1_params);

    /* Private data passed to the endpoint must be valid throughout the scope of protocomm endpoint. This need not be static, i.e., could be dynamically allocated and freed at the time of endpoint removal. */
    static uint32_t priv_data = 1234;

    /* Add a new endpoint for the protocomm instance identified by a unique name, and register a handler function along with the private data to be passed at the time of handler execution. Multiple endpoints can be added as long as they are identified by unique names. */
    protocomm_add_endpoint(pc, "echo_req_endpoint",
                           echo_req_handler, (void *) &priv_data);
    return pc;
}

/* The example function for stopping a protocomm instance. */
void stop_pc(protocomm_t *pc)
{
    /* Remove the endpoint identified by its unique name. */
    protocomm_remove_endpoint(pc, "echo_req_endpoint");

    /* Remove the security endpoint identified by its name. */
    protocomm_unset_security(pc, "security_endpoint");

    /* Stop the HTTP server. */
    protocomm_httpd_stop(pc);

    /* Delete, namely deallocate the protocomm instance. */
    protocomm_delete(pc);
}
```

## Bluetooth LE Transport Example with Security 0

For sample usage, see [network\_provisioning/src/scheme\_ble.c](https://github.com/espressif/idf-extra-components/blob/master/network_provisioning/src/scheme_ble.c).

```c
/* The example function for launching a secure protocomm instance over Bluetooth LE. */
protocomm_t *start_pc()
{
    protocomm_t *pc = protocomm_new();

    /* Endpoint UUIDs */
    protocomm_ble_name_uuid_t nu_lookup_table[] = {
        {"security_endpoint", 0xFF51},
        {"echo_req_endpoint", 0xFF52}
    };

    /* Config for protocomm_ble_start(). */
    protocomm_ble_config_t config = {
        .service_uuid = {
            /* LSB <---------------------------------------
            * ---------------------------------------> MSB */
            0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
            0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        },
        .nu_lookup_count = sizeof(nu_lookup_table)/sizeof(nu_lookup_table[0]),
        .nu_lookup = nu_lookup_table
    };

    /* Start protocomm layer on top of Bluetooth LE. */
    protocomm_ble_start(pc, &config);

    /* For protocomm_security0, Proof of Possession is not used, and can be kept NULL. */
    protocomm_set_security(pc, "security_endpoint", &protocomm_security0, NULL);
    protocomm_add_endpoint(pc, "echo_req_endpoint", echo_req_handler, NULL);
    return pc;
}

/* The example function for stopping a protocomm instance. */
void stop_pc(protocomm_t *pc)
{
    protocomm_remove_endpoint(pc, "echo_req_endpoint");
    protocomm_unset_security(pc, "security_endpoint");

    /* Stop the Bluetooth LE protocomm service. */
    protocomm_ble_stop(pc);

    protocomm_delete(pc);
}
```

## API Reference

### Header File

- [components/protocomm/include/common/protocomm.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/common/protocomm.h)
- This header file can be included with:
	> ```c
	> #include "protocomm.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Functions

\*protocomm\_new(void) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv413protocomm_newv "Permalink to this definition")  

Create a new protocomm instance.

This API will return a new dynamically allocated protocomm instance with all elements of the protocomm\_t structure initialized to NULL.

Returns:

- protocomm\_t\*: On success
- NULL: No memory for allocating new instance

void protocomm\_delete( \*pc) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv416protocomm_deleteP11protocomm_t "Permalink to this definition")  

Delete a protocomm instance.

This API will deallocate a protocomm instance that was created using `protocomm_new()`.

Parameters:

**pc** -- **\[in\]** Pointer to the protocomm instance to be deleted

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_add\_endpoint( \*pc, const char \*ep\_name, h, void \*priv\_data) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv422protocomm_add_endpointP11protocomm_tPKc23protocomm_req_handler_tPv "Permalink to this definition")  

Add endpoint request handler for a protocomm instance.

This API will bind an endpoint handler function to the specified endpoint name, along with any private data that needs to be pass to the handler at the time of call.

Note

- An endpoint must be bound to a valid protocomm instance, created using `protocomm_new()`.
- This function internally calls the registered `add_endpoint()` function of the selected transport which is a member of the protocomm\_t instance structure.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string
- **h** -- **\[in\]** Endpoint handler function
- **priv\_data** -- **\[in\]** Pointer to private data to be passed as a parameter to the handler function on call. Pass NULL if not needed.

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Error adding endpoint / Endpoint with this name already exists
- ESP\_ERR\_NO\_MEM: Error allocating endpoint resource
- ESP\_ERR\_INVALID\_ARG: Null instance/name/handler arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_remove\_endpoint( \*pc, const char \*ep\_name) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv425protocomm_remove_endpointP11protocomm_tPKc "Permalink to this definition")  

Remove endpoint request handler for a protocomm instance.

This API will remove a registered endpoint handler identified by an endpoint name.

Note

- This function internally calls the registered `remove_endpoint()` function which is a member of the protocomm\_t instance structure.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string

Returns:

- ESP\_OK: Success
- ESP\_ERR\_NOT\_FOUND: Endpoint with specified name doesn't exist
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_open\_session( \*pc, uint32\_t session\_id) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv422protocomm_open_sessionP11protocomm_t8uint32_t "Permalink to this definition")  

Allocates internal resources for new transport session.

Note

- An endpoint must be bound to a valid protocomm instance, created using `protocomm_new()`.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **session\_id** -- **\[in\]** Unique ID for a communication session

Returns:

- ESP\_OK: Request handled successfully
- ESP\_ERR\_NO\_MEM: Error allocating internal resource
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_close\_session( \*pc, uint32\_t session\_id) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv423protocomm_close_sessionP11protocomm_t8uint32_t "Permalink to this definition")  

Frees internal resources used by a transport session.

Note

- An endpoint must be bound to a valid protocomm instance, created using `protocomm_new()`.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **session\_id** -- **\[in\]** Unique ID for a communication session

Returns:

- ESP\_OK: Request handled successfully
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_req\_handle( \*pc, const char \*ep\_name, uint32\_t session\_id, const uint8\_t \*inbuf, ssize\_t inlen, uint8\_t \*\*outbuf, ssize\_t \*outlen) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv420protocomm_req_handleP11protocomm_tPKc8uint32_tPK7uint8_t7ssize_tPP7uint8_tP7ssize_t "Permalink to this definition")  

Calls the registered handler of an endpoint session for processing incoming data and generating the response.

Note

- An endpoint must be bound to a valid protocomm instance, created using `protocomm_new()`.
- Resulting output buffer must be deallocated by the caller.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string
- **session\_id** -- **\[in\]** Unique ID for a communication session
- **inbuf** -- **\[in\]** Input buffer contains input request data which is to be processed by the registered handler
- **inlen** -- **\[in\]** Length of the input buffer
- **outbuf** -- **\[out\]** Pointer to internally allocated output buffer, where the resulting response data output from the registered handler is to be stored
- **outlen** -- **\[out\]** Buffer length of the allocated output buffer

Returns:

- ESP\_OK: Request handled successfully
- ESP\_FAIL: Internal error in execution of registered handler
- ESP\_ERR\_NO\_MEM: Error allocating internal resource
- ESP\_ERR\_NOT\_FOUND: Endpoint with specified name doesn't exist
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_set\_security( \*pc, const char \*ep\_name, const \*sec, const void \*sec\_params) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv422protocomm_set_securityP11protocomm_tPKcPK20protocomm_security_tPKv "Permalink to this definition")  

Add endpoint security for a protocomm instance.

This API will bind a security session establisher to the specified endpoint name, along with any proof of possession that may be required for authenticating a session client.

Note

- An endpoint must be bound to a valid protocomm instance, created using `protocomm_new()`.
- The choice of security can be any `protocomm_security_t` instance. Choices `protocomm_security0` and `protocomm_security1` and `protocomm_security2` are readily available.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string
- **sec** -- **\[in\]** Pointer to endpoint security instance
- **sec\_params** -- **\[in\]** Pointer to security params (NULL if not needed) The pointer should contain the security params struct of appropriate security version. For protocomm security version 1 and 2 sec\_params should contain pointer to struct of type protocomm\_security1\_params\_t and protocmm\_security2\_params\_t respectively. The contents of this pointer must be valid till the security session has been running and is not closed.

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Error adding endpoint / Endpoint with this name already exists
- ESP\_ERR\_INVALID\_STATE: Security endpoint already set
- ESP\_ERR\_NO\_MEM: Error allocating endpoint resource
- ESP\_ERR\_INVALID\_ARG: Null instance/name/handler arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_unset\_security( \*pc, const char \*ep\_name) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv424protocomm_unset_securityP11protocomm_tPKc "Permalink to this definition")  

Remove endpoint security for a protocomm instance.

This API will remove a registered security endpoint identified by an endpoint name.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string

Returns:

- ESP\_OK: Success
- ESP\_ERR\_NOT\_FOUND: Endpoint with specified name doesn't exist
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_set\_version( \*pc, const char \*ep\_name, const char \*version) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv421protocomm_set_versionP11protocomm_tPKcPKc "Permalink to this definition")  

Set endpoint for version verification.

This API can be used for setting an application specific protocol version which can be verified by clients through the endpoint.

Note

- An endpoint must be bound to a valid protocomm instance, created using `protocomm_new()`.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string
- **version** -- **\[in\]** Version identifier(name) string

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Error adding endpoint / Endpoint with this name already exists
- ESP\_ERR\_INVALID\_STATE: Version endpoint already set
- ESP\_ERR\_NO\_MEM: Error allocating endpoint resource
- ESP\_ERR\_INVALID\_ARG: Null instance/name/handler arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_unset\_version( \*pc, const char \*ep\_name) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv423protocomm_unset_versionP11protocomm_tPKc "Permalink to this definition")  

Remove version verification endpoint from a protocomm instance.

This API will remove a registered version endpoint identified by an endpoint name.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **ep\_name** -- **\[in\]** Endpoint identifier(name) string

Returns:

- ESP\_OK: Success
- ESP\_ERR\_NOT\_FOUND: Endpoint with specified name doesn't exist
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_get\_sec\_version( \*pc, int \*sec\_ver, uint8\_t \*sec\_patch\_ver) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv425protocomm_get_sec_versionP11protocomm_tPiP7uint8_t "Permalink to this definition")  

Get the security version of the protocomm instance.

This API will return the security version of the protocomm instance.

Parameters:

- **pc** -- **\[in\]** Pointer to the protocomm instance
- **sec\_ver** -- **\[out\]** Pointer to the security version
- **sec\_patch\_ver** -- **\[out\]** Pointer to the security patch version

Returns:

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_ARG: Null instance/name arguments

### Type Definitions

typedef [esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*protocomm\_req\_handler\_t)(uint32\_t session\_id, const uint8\_t \*inbuf, ssize\_t inlen, uint8\_t \*\*outbuf, ssize\_t \*outlen, void \*priv\_data) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv423protocomm_req_handler_t "Permalink to this definition")  

Function prototype for protocomm endpoint handler.

typedef struct protocomm protocomm\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv411protocomm_t "Permalink to this definition")  

This structure corresponds to a unique instance of protocomm returned when the API `protocomm_new()` is called. The remaining Protocomm APIs require this object as the first parameter.

Note

Structure of the protocomm object is kept private

### Header File

- [components/protocomm/include/security/protocomm\_security.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/security/protocomm_security.h)
- This header file can be included with:
	> ```c
	> #include "protocomm_security.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Structures

struct protocomm\_security1\_params [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv426protocomm_security1_params "Permalink to this definition")  

Protocomm Security 1 parameters: Proof Of Possession.

Public Members

const uint8\_t \*data [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N26protocomm_security1_params4dataE "Permalink to this definition")  

Pointer to buffer containing the proof of possession data

uint16\_t len [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N26protocomm_security1_params3lenE "Permalink to this definition")  

Length (in bytes) of the proof of possession data

struct protocomm\_security2\_params [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv426protocomm_security2_params "Permalink to this definition")  

Protocomm Security 2 parameters: Salt and Verifier.

Public Members

const char \*salt [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N26protocomm_security2_params4saltE "Permalink to this definition")  

Pointer to the buffer containing the salt

uint16\_t salt\_len [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N26protocomm_security2_params8salt_lenE "Permalink to this definition")  

Length (in bytes) of the salt

const char \*verifier [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N26protocomm_security2_params8verifierE "Permalink to this definition")  

Pointer to the buffer containing the verifier

uint16\_t verifier\_len [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N26protocomm_security2_params12verifier_lenE "Permalink to this definition")  

Length (in bytes) of the verifier

struct protocomm\_security [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv418protocomm_security "Permalink to this definition")  

Protocomm security object structure.

The member functions are used for implementing secure protocomm sessions.

Note

This structure should not have any dynamic members to allow re-entrancy

Public Members

int ver [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security3verE "Permalink to this definition")  

Unique version number of security implementation

uint8\_t patch\_ver [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security9patch_verE "Permalink to this definition")  

Patch version number of security implementation

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*init)( \*handle) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security4initE "Permalink to this definition")  

Function for initializing/allocating security infrastructure

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*cleanup)( handle) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security7cleanupE "Permalink to this definition")  

Function for deallocating security infrastructure

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*new\_transport\_session)( handle, uint32\_t session\_id) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security21new_transport_sessionE "Permalink to this definition")  

Starts new secure transport session with specified ID

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*close\_transport\_session)( handle, uint32\_t session\_id) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security23close_transport_sessionE "Permalink to this definition")  

Closes a secure transport session with specified ID

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*security\_req\_handler)( handle, const void \*sec\_params, uint32\_t session\_id, const uint8\_t \*inbuf, ssize\_t inlen, uint8\_t \*\*outbuf, ssize\_t \*outlen, void \*priv\_data) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security20security_req_handlerE "Permalink to this definition")  

Handler function for authenticating connection request and establishing secure session

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*encrypt)( handle, uint32\_t session\_id, const uint8\_t \*inbuf, ssize\_t inlen, uint8\_t \*\*outbuf, ssize\_t \*outlen) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security7encryptE "Permalink to this definition")  

Function which implements the encryption algorithm

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") (\*decrypt)( handle, uint32\_t session\_id, const uint8\_t \*inbuf, ssize\_t inlen, uint8\_t \*\*outbuf, ssize\_t \*outlen) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N18protocomm_security7decryptE "Permalink to this definition")  

Function which implements the decryption algorithm

### Type Definitions

typedef struct protocomm\_security1\_params\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv428protocomm_security1_params_t "Permalink to this definition")  

Protocomm Security 1 parameters: Proof Of Possession.

typedef struct protocomm\_security2\_params\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv428protocomm_security2_params_t "Permalink to this definition")  

Protocomm Security 2 parameters: Salt and Verifier.

typedef void \*protocomm\_security\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv427protocomm_security_handle_t "Permalink to this definition")  

typedef struct protocomm\_security\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv420protocomm_security_t "Permalink to this definition")  

Protocomm security object structure.

The member functions are used for implementing secure protocomm sessions.

Note

This structure should not have any dynamic members to allow re-entrancy

### Enumerations

enum protocomm\_security\_session\_event\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv434protocomm_security_session_event_t "Permalink to this definition")  

Events generated by the protocomm security layer.

These events are generated while establishing secured session.

*Values:*

enumerator PROTOCOMM\_SECURITY\_SESSION\_SETUP\_OK [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N34protocomm_security_session_event_t35PROTOCOMM_SECURITY_SESSION_SETUP_OKE "Permalink to this definition")  

Secured session established successfully

enumerator PROTOCOMM\_SECURITY\_SESSION\_INVALID\_SECURITY\_PARAMS [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N34protocomm_security_session_event_t50PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMSE "Permalink to this definition")  

Received invalid (NULL) security parameters (username / client public-key)

enumerator PROTOCOMM\_SECURITY\_SESSION\_CREDENTIALS\_MISMATCH [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N34protocomm_security_session_event_t47PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCHE "Permalink to this definition")  

Received incorrect credentials (username / PoP)

### Header File

- [components/protocomm/include/security/protocomm\_security0.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/security/protocomm_security0.h)
- This header file can be included with:
	> ```c
	> #include "protocomm_security0.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Header File

- [components/protocomm/include/security/protocomm\_security1.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/security/protocomm_security1.h)
- This header file can be included with:
	> ```c
	> #include "protocomm_security1.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Header File

- [components/protocomm/include/security/protocomm\_security2.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/security/protocomm_security2.h)
- This header file can be included with:
	> ```c
	> #include "protocomm_security2.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Header File

- [components/protocomm/include/crypto/srp6a/esp\_srp.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/crypto/srp6a/esp_srp.h)
- This header file can be included with:
	> ```c
	> #include "esp_srp.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Functions

\*esp\_srp\_init( ng) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv412esp_srp_init13esp_ng_type_t "Permalink to this definition")  

Initialize srp context for given NG type.

Note

the handle gets freed with `esp_srp_free`

Parameters:

**ng** -- NG type given by `esp_ng_type_t`

Returns:

esp\_srp\_handle\_t\* srp handle

void esp\_srp\_free( \*hd) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv412esp_srp_freeP16esp_srp_handle_t "Permalink to this definition")  

free esp\_srp\_context

Parameters:

**hd** -- handle to be free

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_srp\_srv\_pubkey( \*hd, const char \*username, int username\_len, const char \*pass, int pass\_len, int salt\_len, char \*\*bytes\_B, int \*len\_B, char \*\*bytes\_salt) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv418esp_srp_srv_pubkeyP16esp_srp_handle_tPKciPKciiPPcPiPPc "Permalink to this definition")  

Returns B (pub key) and salt. \[Step2.b\].

Note

\*bytes\_B MUST NOT BE FREED BY THE CALLER

Note

\*bytes\_salt MUST NOT BE FREE BY THE CALLER

Parameters:

- **hd** -- esp\_srp handle
- **username** -- Username not expected NULL terminated
- **username\_len** -- Username length
- **pass** -- Password not expected to be NULL terminated
- **pass\_len** -- Password length
- **salt\_len** -- Salt length
- **bytes\_B** -- Public Key returned
- **len\_B** -- Length of the public key
- **bytes\_salt** -- Salt bytes generated

Returns:

esp\_err\_t ESP\_OK on success, appropriate error otherwise

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_srp\_gen\_salt\_verifier(const char \*username, int username\_len, const char \*pass, int pass\_len, char \*\*bytes\_salt, int salt\_len, char \*\*verifier, int \*verifier\_len) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv425esp_srp_gen_salt_verifierPKciPKciPPciPPcPi "Permalink to this definition")  

Generate salt-verifier pair, given username, password and salt length.

Note

if API has returned ESP\_OK, salt and verifier generated need to be freed by caller

Note

Usually, username and password are not saved on the device. Rather salt and verifier are generated outside the device and are embedded. this convenience API can be used to generate salt and verifier on the fly for development use case. OR for devices which intentionally want to generate different password each time and can send it to the client securely. e.g., a device has a display and it shows the pin

Parameters:

- **username** -- **\[in\]** username
- **username\_len** -- **\[in\]** length of the username
- **pass** -- **\[in\]** password
- **pass\_len** -- **\[in\]** length of the password
- **bytes\_salt** -- **\[out\]** generated salt on successful generation, or NULL
- **salt\_len** -- **\[in\]** salt length
- **verifier** -- **\[out\]** generated verifier on successful generation, or NULL
- **verifier\_len** -- **\[out\]** length of the generated verifier

Returns:

esp\_err\_t ESP\_OK on success, appropriate error otherwise

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_srp\_set\_salt\_verifier( \*hd, const char \*salt, int salt\_len, const char \*verifier, int verifier\_len) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv425esp_srp_set_salt_verifierP16esp_srp_handle_tPKciPKci "Permalink to this definition")  

Set the Salt and Verifier pre-generated for a given password. This should be used only if the actual password is not available. The public key can then be generated using and not

Parameters:

- **hd** -- esp\_srp\_handle
- **salt** -- pre-generated salt bytes
- **salt\_len** -- length of the salt bytes
- **verifier** -- pre-generated verifier
- **verifier\_len** -- length of the verifier bytes

Returns:

esp\_err\_t ESP\_OK on success, appropriate error otherwise

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_srp\_srv\_pubkey\_from\_salt\_verifier( \*hd, char \*\*bytes\_B, int \*len\_B) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv437esp_srp_srv_pubkey_from_salt_verifierP16esp_srp_handle_tPPcPi "Permalink to this definition")  

Returns B (pub key)\[Step2.b\] when the salt and verifier are set using

Note

\*bytes\_B MUST NOT BE FREED BY THE CALLER

Parameters:

- **hd** -- esp\_srp handle
- **bytes\_B** -- Key returned to the called
- **len\_B** -- Length of the key returned

Returns:

esp\_err\_t ESP\_OK on success, appropriate error otherwise

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_srp\_get\_session\_key( \*hd, char \*bytes\_A, int len\_A, char \*\*bytes\_key, uint16\_t \*len\_key) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv423esp_srp_get_session_keyP16esp_srp_handle_tPciPPcP8uint16_t "Permalink to this definition")  

Get session key in `*bytes_key` given by len in `*len_key`. \[Step2.c\].

This calculated session key is used for further communication given the proofs are exchanged/authenticated with `esp_srp_exchange_proofs`

Note

\*bytes\_key MUST NOT BE FREED BY THE CALLER

Parameters:

- **hd** -- esp\_srp handle
- **bytes\_A** -- Private Key
- **len\_A** -- Private Key length
- **bytes\_key** -- Key returned to the caller
- **len\_key** -- length of the key in \*bytes\_key

Returns:

esp\_err\_t ESP\_OK on success, appropriate error otherwise

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") esp\_srp\_exchange\_proofs( \*hd, char \*username, uint16\_t username\_len, char \*bytes\_user\_proof, char \*bytes\_host\_proof) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv423esp_srp_exchange_proofsP16esp_srp_handle_tPc8uint16_tPcPc "Permalink to this definition")  

Complete the authentication. If this step fails, the session\_key exchanged should not be used.

This is the final authentication step in SRP algorithm \[Step4.1, Step4.b, Step4.c\]

Parameters:

- **hd** -- esp\_srp handle
- **username** -- Username not expected NULL terminated
- **username\_len** -- Username length
- **bytes\_user\_proof** -- param in
- **bytes\_host\_proof** -- parameter out (should be SHA512\_DIGEST\_LENGTH) bytes in size

Returns:

esp\_err\_t ESP\_OK if user's proof is ok and subsequently bytes\_host\_proof is populated with our own proof.

### Type Definitions

typedef struct esp\_srp\_handle esp\_srp\_handle\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv416esp_srp_handle_t "Permalink to this definition")  

esp\_srp handle as the result of `esp_srp_init`

The handle is returned by `esp_srp_init` on successful init. It is then passed for subsequent API calls as an argument. `esp_srp_free` can be used to clean up the handle. After `esp_srp_free` the handle becomes invalid.

### Enumerations

enum esp\_ng\_type\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv413esp_ng_type_t "Permalink to this definition")  

Large prime+generator to be used for the algorithm.

*Values:*

enumerator ESP\_NG\_3072 [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N13esp_ng_type_t11ESP_NG_3072E "Permalink to this definition")  

### Header File

- [components/protocomm/include/transports/protocomm\_httpd.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/transports/protocomm_httpd.h)
- This header file can be included with:
	> ```c
	> #include "protocomm_httpd.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_httpd\_start( \*pc, const \*config) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv421protocomm_httpd_startP11protocomm_tPK24protocomm_httpd_config_t "Permalink to this definition")  

Start HTTPD protocomm transport.

This API internally creates a framework to allow endpoint registration and security configuration for the protocomm.

Note

This is a singleton. ie. Protocomm can have multiple instances, but only one instance can be bound to an HTTP transport layer.

Parameters:

- **pc** -- **\[in\]** Protocomm instance pointer obtained from protocomm\_new()
- **config** -- **\[in\]** Pointer to config structure for initializing HTTP server

Returns:

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_ARG: Null arguments
- ESP\_ERR\_NOT\_SUPPORTED: Transport layer bound to another protocomm instance
- ESP\_ERR\_INVALID\_STATE: Transport layer already bound to this protocomm instance
- ESP\_ERR\_NO\_MEM: Memory allocation for server resource failed
- ESP\_ERR\_HTTPD\_\*: HTTP server error on start

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_httpd\_stop( \*pc) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv420protocomm_httpd_stopP11protocomm_t "Permalink to this definition")  

Stop HTTPD protocomm transport.

This API cleans up the HTTPD transport protocomm and frees all the handlers registered with the protocomm.

Parameters:

**pc** -- **\[in\]** Same protocomm instance that was passed to protocomm\_httpd\_start()

Returns:

- ESP\_OK: Success
- ESP\_ERR\_INVALID\_ARG: Null / incorrect protocomm instance pointer

### Unions

union protocomm\_httpd\_config\_data\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv429protocomm_httpd_config_data_t "Permalink to this definition")  

*#include <protocomm\_httpd.h>*

Protocomm HTTPD Configuration Data

Public Members

void \*handle [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N29protocomm_httpd_config_data_t6handleE "Permalink to this definition")  

HTTP Server Handle, if ext\_handle\_provided is set to true

config [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N29protocomm_httpd_config_data_t6configE "Permalink to this definition")  

HTTP Server Configuration, if a server is not already active

### Structures

struct protocomm\_http\_server\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv430protocomm_http_server_config_t "Permalink to this definition")  

Config parameters for protocomm HTTP server.

Public Members

uint16\_t port [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N30protocomm_http_server_config_t4portE "Permalink to this definition")  

Port on which the HTTP server will listen

size\_t stack\_size [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N30protocomm_http_server_config_t10stack_sizeE "Permalink to this definition")  

Stack size of server task, adjusted depending upon stack usage of endpoint handler

unsigned task\_priority [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N30protocomm_http_server_config_t13task_priorityE "Permalink to this definition")  

Priority of server task

struct protocomm\_httpd\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv424protocomm_httpd_config_t "Permalink to this definition")  

Config parameters for protocomm HTTP server.

Public Members

bool ext\_handle\_provided [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N24protocomm_httpd_config_t19ext_handle_providedE "Permalink to this definition")  

Flag to indicate of an external HTTP Server Handle has been provided. In such as case, protocomm will use the same HTTP Server and not start a new one internally.

data [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N24protocomm_httpd_config_t4dataE "Permalink to this definition")  

Protocomm HTTPD Configuration Data

### Macros

PROTOCOMM\_HTTPD\_DEFAULT\_CONFIG() [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#c.PROTOCOMM_HTTPD_DEFAULT_CONFIG "Permalink to this definition")  

### Header File

- [components/protocomm/include/transports/protocomm\_ble.h](https://github.com/espressif/esp-idf/blob/47faecc3/components/protocomm/include/transports/protocomm_ble.h)
- This header file can be included with:
	> ```c
	> #include "protocomm_ble.h"
	> ```
- This header file is a part of the API provided by the `protocomm` component. To declare that your component depends on `protocomm`, add the following to your CMakeLists.txt:
	> ```c
	> REQUIRES protocomm
	> ```
	> 
	> or
	> 
	> ```c
	> PRIV_REQUIRES protocomm
	> ```

### Functions

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_ble\_start( \*pc, const \*config) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv419protocomm_ble_startP11protocomm_tPK22protocomm_ble_config_t "Permalink to this definition")  

Start Bluetooth Low Energy based transport layer for provisioning.

Initialize and start required BLE service for provisioning. This includes the initialization for characteristics/service for BLE.

Parameters:

- **pc** -- **\[in\]** Protocomm instance pointer obtained from protocomm\_new()
- **config** -- **\[in\]** Pointer to config structure for initializing BLE

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Simple BLE start error
- ESP\_ERR\_NO\_MEM: Error allocating memory for internal resources
- ESP\_ERR\_INVALID\_STATE: Error in ble config
- ESP\_ERR\_INVALID\_ARG: Null arguments

[esp\_err\_t](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html#_CPPv49esp_err_t "esp_err_t") protocomm\_ble\_stop( \*pc) [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv418protocomm_ble_stopP11protocomm_t "Permalink to this definition")  

Stop Bluetooth Low Energy based transport layer for provisioning.

Stops service/task responsible for BLE based interactions for provisioning

Note

You might want to optionally reclaim memory from Bluetooth. Refer to the documentation of `esp_bt_mem_release` in that case.

Parameters:

**pc** -- **\[in\]** Same protocomm instance that was passed to protocomm\_ble\_start()

Returns:

- ESP\_OK: Success
- ESP\_FAIL: Simple BLE stop error
- ESP\_ERR\_INVALID\_ARG: Null / incorrect protocomm instance

### Structures

struct name\_uuid [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv49name_uuid "Permalink to this definition")  

This structure maps handler required by protocomm layer to UUIDs which are used to uniquely identify BLE characteristics from a smartphone or a similar client device.

Public Members

const char \*name [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N9name_uuid4nameE "Permalink to this definition")  

Name of the handler, which is passed to protocomm layer

uint16\_t uuid [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N9name_uuid4uuidE "Permalink to this definition")  

UUID to be assigned to the BLE characteristic which is mapped to the handler

struct protocomm\_ble\_event\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv421protocomm_ble_event_t "Permalink to this definition")  

Structure for BLE events in Protocomm.

Public Members

uint16\_t evt\_type [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N21protocomm_ble_event_t8evt_typeE "Permalink to this definition")  

This field indicates the type of BLE event that occurred.

uint16\_t conn\_handle [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N21protocomm_ble_event_t11conn_handleE "Permalink to this definition")  

The handle of the relevant connection.

uint16\_t conn\_status [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N21protocomm_ble_event_t11conn_statusE "Permalink to this definition")  

The status of the connection attempt; o 0: the connection was successfully established. o BLE host error code: the connection attempt failed for the specified reason.

uint16\_t disconnect\_reason [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N21protocomm_ble_event_t17disconnect_reasonE "Permalink to this definition")  

Return code indicating the reason for the disconnect.

struct protocomm\_ble\_config [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv420protocomm_ble_config "Permalink to this definition")  

Config parameters for protocomm BLE service.

Public Members

char device\_name\[MAX\_BLE\_DEVNAME\_LEN + 1\] [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config11device_nameE "Permalink to this definition")  

BLE device name being broadcast at the time of provisioning

uint8\_t service\_uuid\[BLE\_UUID128\_VAL\_LENGTH\] [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config12service_uuidE "Permalink to this definition")  

128 bit UUID of the provisioning service

uint8\_t \*manufacturer\_data [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config17manufacturer_dataE "Permalink to this definition")  

BLE device manufacturer data pointer in advertisement

ssize\_t manufacturer\_data\_len [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config21manufacturer_data_lenE "Permalink to this definition")  

BLE device manufacturer data length in advertisement

ssize\_t nu\_lookup\_count [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config15nu_lookup_countE "Permalink to this definition")  

Number of entries in the Name-UUID lookup table

\*nu\_lookup [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config9nu_lookupE "Permalink to this definition")  

Pointer to the Name-UUID lookup table

unsigned ble\_bonding [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config11ble_bondingE "Permalink to this definition")  

BLE bonding

unsigned ble\_sm\_sc [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config9ble_sm_scE "Permalink to this definition")  

BLE security flag

unsigned ble\_link\_encryption [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config19ble_link_encryptionE "Permalink to this definition")  

BLE security flag

uint8\_t \*ble\_addr [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config8ble_addrE "Permalink to this definition")  

BLE address

unsigned keep\_ble\_on [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config11keep_ble_onE "Permalink to this definition")  

Flag to keep BLE on

unsigned ble\_notify [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N20protocomm_ble_config10ble_notifyE "Permalink to this definition")  

BLE characteristic notify flag

### Macros

MAX\_BLE\_DEVNAME\_LEN [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#c.MAX_BLE_DEVNAME_LEN "Permalink to this definition")  

BLE device name cannot be larger than this value 31 bytes (max scan response size) - 1 byte (length) - 1 byte (type) = 29 bytes

BLE\_UUID128\_VAL\_LENGTH [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#c.BLE_UUID128_VAL_LENGTH "Permalink to this definition")  

MAX\_BLE\_MANUFACTURER\_DATA\_LEN [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#c.MAX_BLE_MANUFACTURER_DATA_LEN "Permalink to this definition")  

Theoretically, the limit for max manufacturer length remains same as BLE device name i.e. 31 bytes (max scan response size) - 1 byte (length) - 1 byte (type) = 29 bytes However, manufacturer data goes along with BLE device name in scan response. So, it is important to understand the actual length should be smaller than (29 - (BLE device name length) - 2).

BLE\_ADDR\_LEN [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#c.BLE_ADDR_LEN "Permalink to this definition")  

### Type Definitions

typedef struct protocomm\_ble\_name\_uuid\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv425protocomm_ble_name_uuid_t "Permalink to this definition")  

This structure maps handler required by protocomm layer to UUIDs which are used to uniquely identify BLE characteristics from a smartphone or a similar client device.

typedef struct protocomm\_ble\_config\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv422protocomm_ble_config_t "Permalink to this definition")  

Config parameters for protocomm BLE service.

### Enumerations

enum protocomm\_transport\_ble\_event\_t [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv431protocomm_transport_ble_event_t "Permalink to this definition")  

Events generated by BLE transport.

These events are generated when the BLE transport is paired and disconnected.

*Values:*

enumerator PROTOCOMM\_TRANSPORT\_BLE\_CONNECTED [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N31protocomm_transport_ble_event_t33PROTOCOMM_TRANSPORT_BLE_CONNECTEDE "Permalink to this definition")  

enumerator PROTOCOMM\_TRANSPORT\_BLE\_DISCONNECTED [](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/protocomm.html#_CPPv4N31protocomm_transport_ble_event_t36PROTOCOMM_TRANSPORT_BLE_DISCONNECTEDE "Permalink to this definition")