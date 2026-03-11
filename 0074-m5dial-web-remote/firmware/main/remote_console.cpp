#include "remote_console.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "esp_console.h"
#include "esp_err.h"

#include "remote_client.h"
#include "remote_config.h"

namespace {

void print_usage() {
  std::printf("usage:\n");
  std::printf("  remote status\n");
  std::printf("  remote set-url <ws://host:port/ws/device>\n");
  std::printf("  remote set-id <device-id>\n");
  std::printf("  remote connect\n");
  std::printf("  remote disconnect\n");
  std::printf("  remote clear\n");
}

const char* state_name(RemoteClientState state) {
  switch (state) {
    case RemoteClientState::kIdle:
      return "IDLE";
    case RemoteClientState::kWaitingForWifi:
      return "WAIT_WIFI";
    case RemoteClientState::kConnecting:
      return "CONNECTING";
    case RemoteClientState::kConnected:
      return "CONNECTED";
    case RemoteClientState::kError:
      return "ERROR";
  }
  return "?";
}

int cmd_remote(int argc, char** argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  if (std::strcmp(argv[1], "status") == 0) {
    RemoteConfig cfg = {};
    const esp_err_t load_err = remote_config_load(&cfg);
    if (load_err != ESP_OK) {
      std::printf("remote status: config load failed: %s\n", esp_err_to_name(load_err));
      return 1;
    }
    RemoteClientStatus status = {};
    remote_client_get_status(&status);
    std::printf("state=%s desired=%s\n", state_name(status.state), status.desired_connected ? "yes" : "no");
    std::printf("url=%s\n", cfg.url[0] ? cfg.url : "-");
    std::printf("device_id=%s\n", cfg.device_id[0] ? cfg.device_id : "-");
    std::printf("tx=%" PRIu32 " rx=%" PRIu32 "\n", status.tx_count, status.rx_count);
    std::printf("last_error=%s\n", status.last_error[0] ? status.last_error : "-");
    std::printf("last_message=%s\n", status.last_message[0] ? status.last_message : "-");
    return 0;
  }

  if (std::strcmp(argv[1], "set-url") == 0) {
    if (argc < 3) {
      print_usage();
      return 1;
    }
    RemoteConfig cfg = {};
    (void)remote_config_load(&cfg);
    std::snprintf(cfg.url, sizeof(cfg.url), "%s", argv[2]);
    const esp_err_t err = remote_config_save(cfg);
    if (err != ESP_OK) {
      std::printf("remote set-url: %s\n", esp_err_to_name(err));
      return 1;
    }
    remote_client_set_config(cfg);
    std::printf("remote url saved: %s\n", cfg.url);
    return 0;
  }

  if (std::strcmp(argv[1], "set-id") == 0) {
    if (argc < 3) {
      print_usage();
      return 1;
    }
    RemoteConfig cfg = {};
    (void)remote_config_load(&cfg);
    std::snprintf(cfg.device_id, sizeof(cfg.device_id), "%s", argv[2]);
    const esp_err_t err = remote_config_save(cfg);
    if (err != ESP_OK) {
      std::printf("remote set-id: %s\n", esp_err_to_name(err));
      return 1;
    }
    remote_client_set_config(cfg);
    std::printf("remote device id saved: %s\n", cfg.device_id);
    return 0;
  }

  if (std::strcmp(argv[1], "connect") == 0) {
    const esp_err_t err = remote_client_connect();
    if (err != ESP_OK) {
      std::printf("remote connect: %s\n", esp_err_to_name(err));
      return 1;
    }
    std::printf("remote connect requested\n");
    return 0;
  }

  if (std::strcmp(argv[1], "disconnect") == 0) {
    const esp_err_t err = remote_client_disconnect();
    if (err != ESP_OK) {
      std::printf("remote disconnect: %s\n", esp_err_to_name(err));
      return 1;
    }
    std::printf("remote disconnected\n");
    return 0;
  }

  if (std::strcmp(argv[1], "clear") == 0) {
    const esp_err_t disconnect_err = remote_client_disconnect();
    const esp_err_t clear_err = remote_config_clear();
    RemoteConfig cfg = {};
    remote_client_set_config(cfg);
    if (disconnect_err != ESP_OK) {
      std::printf("remote clear: disconnect failed: %s\n", esp_err_to_name(disconnect_err));
      return 1;
    }
    if (clear_err != ESP_OK) {
      std::printf("remote clear: %s\n", esp_err_to_name(clear_err));
      return 1;
    }
    std::printf("remote config cleared\n");
    return 0;
  }

  print_usage();
  return 1;
}

}  // namespace

void remote_console_register() {
  esp_console_cmd_t cmd = {};
  cmd.command = "remote";
  cmd.help = "Remote websocket config: remote status|set-url|set-id|connect|disconnect|clear";
  cmd.func = &cmd_remote;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
