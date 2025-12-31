#include "bt_decode.h"

#include <inttypes.h>
#include <stdio.h>

static const bt_decode_entry_t k_gatt_status[] = {
    {ESP_GATT_OK, "ESP_GATT_OK", "Operation successful"},
    {ESP_GATT_INVALID_HANDLE, "ESP_GATT_INVALID_HANDLE", "Invalid handle"},
    {ESP_GATT_READ_NOT_PERMIT, "ESP_GATT_READ_NOT_PERMIT", "Read operation not permitted"},
    {ESP_GATT_WRITE_NOT_PERMIT, "ESP_GATT_WRITE_NOT_PERMIT", "Write operation not permitted"},
    {ESP_GATT_INVALID_PDU, "ESP_GATT_INVALID_PDU", "Invalid PDU"},
    {ESP_GATT_INSUF_AUTHENTICATION, "ESP_GATT_INSUF_AUTHENTICATION", "Insufficient authentication"},
    {ESP_GATT_REQ_NOT_SUPPORTED, "ESP_GATT_REQ_NOT_SUPPORTED", "Request not supported"},
    {ESP_GATT_INVALID_OFFSET, "ESP_GATT_INVALID_OFFSET", "Invalid offset"},
    {ESP_GATT_INSUF_AUTHORIZATION, "ESP_GATT_INSUF_AUTHORIZATION", "Insufficient authorization"},
    {ESP_GATT_PREPARE_Q_FULL, "ESP_GATT_PREPARE_Q_FULL", "Prepare queue full"},
    {ESP_GATT_NOT_FOUND, "ESP_GATT_NOT_FOUND", "Not found"},
    {ESP_GATT_NOT_LONG, "ESP_GATT_NOT_LONG", "Not long"},
    {ESP_GATT_INSUF_KEY_SIZE, "ESP_GATT_INSUF_KEY_SIZE", "Insufficient key size"},
    {ESP_GATT_INVALID_ATTR_LEN, "ESP_GATT_INVALID_ATTR_LEN", "Invalid attribute length"},
    {ESP_GATT_ERR_UNLIKELY, "ESP_GATT_ERR_UNLIKELY", "Unlikely error"},
    {ESP_GATT_INSUF_ENCRYPTION, "ESP_GATT_INSUF_ENCRYPTION", "Insufficient encryption"},
    {ESP_GATT_UNSUPPORT_GRP_TYPE, "ESP_GATT_UNSUPPORT_GRP_TYPE", "Unsupported group type"},
    {ESP_GATT_INSUF_RESOURCE, "ESP_GATT_INSUF_RESOURCE", "Insufficient resource"},

    {ESP_GATT_NO_RESOURCES, "ESP_GATT_NO_RESOURCES", "No resources"},
    {ESP_GATT_INTERNAL_ERROR, "ESP_GATT_INTERNAL_ERROR", "Internal error"},
    {ESP_GATT_WRONG_STATE, "ESP_GATT_WRONG_STATE", "Wrong state"},
    {ESP_GATT_DB_FULL, "ESP_GATT_DB_FULL", "Database full"},
    {ESP_GATT_BUSY, "ESP_GATT_BUSY", "Busy"},
    {ESP_GATT_ERROR, "ESP_GATT_ERROR", "Generic error (aka 0x85 / 133)"},
    {ESP_GATT_CMD_STARTED, "ESP_GATT_CMD_STARTED", "Command started"},
    {ESP_GATT_ILLEGAL_PARAMETER, "ESP_GATT_ILLEGAL_PARAMETER", "Illegal parameter"},
    {ESP_GATT_PENDING, "ESP_GATT_PENDING", "Operation pending"},
    {ESP_GATT_AUTH_FAIL, "ESP_GATT_AUTH_FAIL", "Authentication failed"},
    {ESP_GATT_MORE, "ESP_GATT_MORE", "More data available"},
    {ESP_GATT_INVALID_CFG, "ESP_GATT_INVALID_CFG", "Invalid configuration"},
    {ESP_GATT_SERVICE_STARTED, "ESP_GATT_SERVICE_STARTED", "Service started"},
    {ESP_GATT_ENCRYPTED_MITM, "ESP_GATT_ENCRYPTED_MITM", "Encrypted, with MITM protection"},
    {ESP_GATT_ENCRYPTED_NO_MITM, "ESP_GATT_ENCRYPTED_NO_MITM", "Encrypted, without MITM protection"},
    {ESP_GATT_NOT_ENCRYPTED, "ESP_GATT_NOT_ENCRYPTED", "Not encrypted"},
    {ESP_GATT_CONGESTED, "ESP_GATT_CONGESTED", "Congested"},
    {ESP_GATT_DUP_REG, "ESP_GATT_DUP_REG", "Duplicate registration"},
    {ESP_GATT_ALREADY_OPEN, "ESP_GATT_ALREADY_OPEN", "Already open"},
    {ESP_GATT_CANCEL, "ESP_GATT_CANCEL", "Operation cancelled"},

    {ESP_GATT_STACK_RSP, "ESP_GATT_STACK_RSP", "Stack response"},
    {ESP_GATT_APP_RSP, "ESP_GATT_APP_RSP", "Application response"},
    {ESP_GATT_UNKNOWN_ERROR, "ESP_GATT_UNKNOWN_ERROR", "Unknown error (app or stack bug)"},
    {ESP_GATT_CCC_CFG_ERR, "ESP_GATT_CCC_CFG_ERR", "CCC descriptor improperly configured"},
    {ESP_GATT_PRC_IN_PROGRESS, "ESP_GATT_PRC_IN_PROGRESS", "Procedure already in progress"},
    {ESP_GATT_OUT_OF_RANGE, "ESP_GATT_OUT_OF_RANGE", "Attribute value out of range"},
};

static const bt_decode_entry_t k_gatt_conn_reason[] = {
    {ESP_GATT_CONN_UNKNOWN, "ESP_GATT_CONN_UNKNOWN", "Unknown connection reason"},
    {ESP_GATT_CONN_L2C_FAILURE, "ESP_GATT_CONN_L2C_FAILURE", "General L2CAP failure"},
    {ESP_GATT_CONN_TIMEOUT, "ESP_GATT_CONN_TIMEOUT", "Connection timeout"},
    {ESP_GATT_CONN_TERMINATE_PEER_USER, "ESP_GATT_CONN_TERMINATE_PEER_USER", "Connection terminated by peer user"},
    {ESP_GATT_CONN_TERMINATE_LOCAL_HOST, "ESP_GATT_CONN_TERMINATE_LOCAL_HOST", "Connection terminated by local host"},
    {ESP_GATT_CONN_FAIL_ESTABLISH, "ESP_GATT_CONN_FAIL_ESTABLISH", "Failure to establish connection"},
    {ESP_GATT_CONN_LMP_TIMEOUT, "ESP_GATT_CONN_LMP_TIMEOUT", "Connection failed due to LMP response timeout"},
    {ESP_GATT_CONN_CONN_CANCEL, "ESP_GATT_CONN_CONN_CANCEL", "L2CAP connection cancelled"},
    {ESP_GATT_CONN_NONE, "ESP_GATT_CONN_NONE", "No connection to cancel"},
};

static const bt_decode_entry_t k_bt_status[] = {
    {ESP_BT_STATUS_SUCCESS, "ESP_BT_STATUS_SUCCESS", "Success"},
    {ESP_BT_STATUS_FAIL, "ESP_BT_STATUS_FAIL", "Fail"},
    {ESP_BT_STATUS_NOT_READY, "ESP_BT_STATUS_NOT_READY", "Not ready"},
    {ESP_BT_STATUS_NOMEM, "ESP_BT_STATUS_NOMEM", "No memory"},
    {ESP_BT_STATUS_BUSY, "ESP_BT_STATUS_BUSY", "Busy"},
    {ESP_BT_STATUS_DONE, "ESP_BT_STATUS_DONE", "Done"},
    {ESP_BT_STATUS_UNSUPPORTED, "ESP_BT_STATUS_UNSUPPORTED", "Unsupported"},
    {ESP_BT_STATUS_PARM_INVALID, "ESP_BT_STATUS_PARM_INVALID", "Invalid parameter"},
    {ESP_BT_STATUS_UNHANDLED, "ESP_BT_STATUS_UNHANDLED", "Unhandled"},
    {ESP_BT_STATUS_AUTH_FAILURE, "ESP_BT_STATUS_AUTH_FAILURE", "Authentication failure"},
    {ESP_BT_STATUS_RMT_DEV_DOWN, "ESP_BT_STATUS_RMT_DEV_DOWN", "Remote device down"},
    {ESP_BT_STATUS_AUTH_REJECTED, "ESP_BT_STATUS_AUTH_REJECTED", "Authentication rejected"},
    {ESP_BT_STATUS_INVALID_STATIC_RAND_ADDR, "ESP_BT_STATUS_INVALID_STATIC_RAND_ADDR", "Invalid static random address"},
    {ESP_BT_STATUS_PENDING, "ESP_BT_STATUS_PENDING", "Pending"},
    {ESP_BT_STATUS_UNACCEPT_CONN_INTERVAL, "ESP_BT_STATUS_UNACCEPT_CONN_INTERVAL", "Unacceptable connection interval"},
    {ESP_BT_STATUS_PARAM_OUT_OF_RANGE, "ESP_BT_STATUS_PARAM_OUT_OF_RANGE", "Parameter out of range"},
    {ESP_BT_STATUS_TIMEOUT, "ESP_BT_STATUS_TIMEOUT", "Timeout"},
    {ESP_BT_STATUS_PEER_LE_DATA_LEN_UNSUPPORTED,
     "ESP_BT_STATUS_PEER_LE_DATA_LEN_UNSUPPORTED",
     "Peer LE data length unsupported"},
    {ESP_BT_STATUS_CONTROL_LE_DATA_LEN_UNSUPPORTED,
     "ESP_BT_STATUS_CONTROL_LE_DATA_LEN_UNSUPPORTED",
     "Controller LE data length unsupported"},
    {ESP_BT_STATUS_ERR_ILLEGAL_PARAMETER_FMT,
     "ESP_BT_STATUS_ERR_ILLEGAL_PARAMETER_FMT",
     "Illegal parameter format"},
    {ESP_BT_STATUS_MEMORY_FULL, "ESP_BT_STATUS_MEMORY_FULL", "Memory full"},
    {ESP_BT_STATUS_EIR_TOO_LARGE, "ESP_BT_STATUS_EIR_TOO_LARGE", "EIR too large"},
};

static const bt_decode_entry_t *find_entry(const bt_decode_entry_t *entries, size_t n, uint32_t value) {
    for (size_t i = 0; i < n; i++) {
        if (entries[i].value == value) return &entries[i];
    }
    return NULL;
}

const char *bt_decode_gatt_status_name(esp_gatt_status_t status) {
    const bt_decode_entry_t *e = find_entry(k_gatt_status, sizeof(k_gatt_status) / sizeof(k_gatt_status[0]), status);
    return e ? e->name : "ESP_GATT_STATUS_UNKNOWN";
}

const char *bt_decode_gatt_status_desc(esp_gatt_status_t status) {
    const bt_decode_entry_t *e = find_entry(k_gatt_status, sizeof(k_gatt_status) / sizeof(k_gatt_status[0]), status);
    return e ? e->desc : "Unknown GATT status";
}

const char *bt_decode_gatt_conn_reason_name(esp_gatt_conn_reason_t reason) {
    const bt_decode_entry_t *e =
        find_entry(k_gatt_conn_reason, sizeof(k_gatt_conn_reason) / sizeof(k_gatt_conn_reason[0]), reason);
    return e ? e->name : "ESP_GATT_CONN_REASON_UNKNOWN";
}

const char *bt_decode_gatt_conn_reason_desc(esp_gatt_conn_reason_t reason) {
    const bt_decode_entry_t *e =
        find_entry(k_gatt_conn_reason, sizeof(k_gatt_conn_reason) / sizeof(k_gatt_conn_reason[0]), reason);
    return e ? e->desc : "Unknown GATT connection reason";
}

const char *bt_decode_bt_status_name(esp_bt_status_t status) {
    const bt_decode_entry_t *e = find_entry(k_bt_status, sizeof(k_bt_status) / sizeof(k_bt_status[0]), status);
    if (e) return e->name;

    if ((uint32_t)status >= ESP_BT_STATUS_BASE_FOR_HCI_ERR) {
        return "ESP_BT_STATUS_HCI_ERR";
    }
    return "ESP_BT_STATUS_UNKNOWN";
}

const char *bt_decode_bt_status_desc(esp_bt_status_t status) {
    const bt_decode_entry_t *e = find_entry(k_bt_status, sizeof(k_bt_status) / sizeof(k_bt_status[0]), status);
    if (e) return e->desc;

    if ((uint32_t)status >= ESP_BT_STATUS_BASE_FOR_HCI_ERR) {
        return "HCI error (see BT spec for meaning)";
    }
    return "Unknown BT status";
}

static void print_entries(const char *title, const bt_decode_entry_t *entries, size_t n, const char *fmt) {
    printf("%s\n", title);
    for (size_t i = 0; i < n; i++) {
        printf(fmt, entries[i].value, entries[i].name, entries[i].desc);
    }
}

void bt_decode_print_all(void) {
    print_entries("GATT status codes (esp_gatt_status_t):",
                  k_gatt_status,
                  sizeof(k_gatt_status) / sizeof(k_gatt_status[0]),
                  "  0x%02" PRIx32 "  %-30s  %s\n");

    print_entries("GATT connection reasons (esp_gatt_conn_reason_t):",
                  k_gatt_conn_reason,
                  sizeof(k_gatt_conn_reason) / sizeof(k_gatt_conn_reason[0]),
                  "  0x%04" PRIx32 "  %-34s  %s\n");

    print_entries("BT status codes (esp_bt_status_t) (non-HCI subset):",
                  k_bt_status,
                  sizeof(k_bt_status) / sizeof(k_bt_status[0]),
                  "  0x%04" PRIx32 "  %-36s  %s\n");

    printf("BT status HCI mapping note:\n");
    printf("  If status >= 0x%04x, it is an HCI error with code (status - 0x%04x).\n",
           (unsigned)ESP_BT_STATUS_BASE_FOR_HCI_ERR,
           (unsigned)ESP_BT_STATUS_BASE_FOR_HCI_ERR);
}
