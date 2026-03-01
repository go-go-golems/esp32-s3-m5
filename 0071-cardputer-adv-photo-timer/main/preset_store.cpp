#include "preset_store.h"

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <set>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_spiffs.h"

namespace {

static const char* TAG = "preset_store_0071";
static const char* kConfigPath = "/spiffs/presets.json";
static const char* kMountPath = "/spiffs";
static const char* kPartitionLabel = "storage";

bool s_spiffs_mounted = false;

TimerConfig build_default_config() {
  TimerConfig cfg;
  cfg.version = 1;

  TimerPreset c41;
  c41.id = "c41-cinestill";
  c41.name = "C41 CineStill CS41";
  c41.steps.push_back(TimerStep{"Pre-wet", 60});
  c41.steps.push_back(TimerStep{"Developer", 210});
  c41.steps.push_back(TimerStep{"Blix", 480});
  c41.steps.push_back(TimerStep{"Wash", 180});
  c41.steps.push_back(TimerStep{"Stabilizer", 60});

  TimerPreset bw;
  bw.id = "bw-standard";
  bw.name = "B&W Standard";
  bw.steps.push_back(TimerStep{"Developer", 540});
  bw.steps.push_back(TimerStep{"Stop Bath", 60});
  bw.steps.push_back(TimerStep{"Fix", 360});
  bw.steps.push_back(TimerStep{"Wash", 300});
  bw.steps.push_back(TimerStep{"Wetting Agent", 30});

  cfg.presets.push_back(c41);
  cfg.presets.push_back(bw);
  cfg.active_preset_id = c41.id;
  return cfg;
}

bool ensure_dir(const char* path) {
  struct stat st = {};
  if (stat(path, &st) == 0) {
    return S_ISDIR(st.st_mode);
  }
  return mkdir(path, 0755) == 0;
}

bool read_file(const char* path, std::string* out) {
  if (!path || !out) return false;
  FILE* f = fopen(path, "rb");
  if (!f) return false;

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }
  const long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return false;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }

  out->assign((size_t)size, '\0');
  if ((size_t)size > 0) {
    const size_t n = fread(out->data(), 1, (size_t)size, f);
    if (n != (size_t)size) {
      fclose(f);
      return false;
    }
  }

  fclose(f);
  return true;
}

bool write_file(const char* path, const std::string& body) {
  if (!path) return false;
  FILE* f = fopen(path, "wb");
  if (!f) return false;
  const size_t n = fwrite(body.data(), 1, body.size(), f);
  fclose(f);
  return n == body.size();
}

bool parse_step(const cJSON* step_obj, TimerStep* out, std::string* err) {
  if (!step_obj || !out) {
    if (err) *err = "invalid step object";
    return false;
  }

  const cJSON* j_name = cJSON_GetObjectItemCaseSensitive(step_obj, "name");
  const cJSON* j_seconds = cJSON_GetObjectItemCaseSensitive(step_obj, "seconds");

  if (!cJSON_IsString(j_name) || !j_name->valuestring || j_name->valuestring[0] == '\0') {
    if (err) *err = "step.name must be a non-empty string";
    return false;
  }
  if (!cJSON_IsNumber(j_seconds) || j_seconds->valuedouble < 1 || j_seconds->valuedouble > 86400) {
    if (err) *err = "step.seconds must be a number in range [1,86400]";
    return false;
  }

  out->name = j_name->valuestring;
  out->seconds = (uint32_t)j_seconds->valuedouble;
  return true;
}

bool parse_preset(const cJSON* preset_obj, TimerPreset* out, std::string* err) {
  if (!preset_obj || !out) {
    if (err) *err = "invalid preset object";
    return false;
  }

  const cJSON* j_id = cJSON_GetObjectItemCaseSensitive(preset_obj, "id");
  const cJSON* j_name = cJSON_GetObjectItemCaseSensitive(preset_obj, "name");
  const cJSON* j_steps = cJSON_GetObjectItemCaseSensitive(preset_obj, "steps");

  if (!cJSON_IsString(j_id) || !j_id->valuestring || j_id->valuestring[0] == '\0') {
    if (err) *err = "preset.id must be a non-empty string";
    return false;
  }
  if (!cJSON_IsString(j_name) || !j_name->valuestring || j_name->valuestring[0] == '\0') {
    if (err) *err = "preset.name must be a non-empty string";
    return false;
  }
  if (!cJSON_IsArray(j_steps) || cJSON_GetArraySize(j_steps) <= 0) {
    if (err) *err = "preset.steps must be a non-empty array";
    return false;
  }

  out->id = j_id->valuestring;
  out->name = j_name->valuestring;
  out->steps.clear();

  const int n = cJSON_GetArraySize(j_steps);
  out->steps.reserve((size_t)n);
  for (int i = 0; i < n; i++) {
    const cJSON* item = cJSON_GetArrayItem(j_steps, i);
    TimerStep step;
    if (!parse_step(item, &step, err)) {
      return false;
    }
    out->steps.push_back(step);
  }

  return true;
}

void normalize_active_preset(TimerConfig* cfg) {
  if (!cfg || cfg->presets.empty()) return;
  const TimerPreset* active = find_preset_by_id(*cfg, cfg->active_preset_id);
  if (!active) {
    cfg->active_preset_id = cfg->presets[0].id;
  }
}

}  // namespace

esp_err_t preset_store_init(bool format_if_mount_failed) {
  if (s_spiffs_mounted) {
    return ESP_OK;
  }

  esp_vfs_spiffs_conf_t conf = {};
  conf.base_path = kMountPath;
  conf.partition_label = kPartitionLabel;
  conf.max_files = 8;
  conf.format_if_mount_failed = format_if_mount_failed;

  esp_err_t err = esp_vfs_spiffs_register(&conf);
  if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
    s_spiffs_mounted = true;
    if (!ensure_dir(kMountPath)) {
      ESP_LOGW(TAG, "failed to ensure mount path exists: %s", kMountPath);
    }
    return ESP_OK;
  }

  ESP_LOGE(TAG, "esp_vfs_spiffs_register failed: %s", esp_err_to_name(err));
  return err;
}

esp_err_t preset_store_parse_json(const char* json, size_t len, TimerConfig* out_cfg, std::string* err_detail) {
  if (!json || !out_cfg) {
    if (err_detail) *err_detail = "invalid arguments";
    return ESP_ERR_INVALID_ARG;
  }

  cJSON* root = cJSON_ParseWithLength(json, len);
  if (!root) {
    if (err_detail) *err_detail = "invalid JSON";
    return ESP_ERR_INVALID_ARG;
  }

  TimerConfig cfg;

  const cJSON* j_version = cJSON_GetObjectItemCaseSensitive(root, "version");
  if (cJSON_IsNumber(j_version) && j_version->valuedouble >= 1) {
    cfg.version = (uint32_t)j_version->valuedouble;
  }

  const cJSON* j_active = cJSON_GetObjectItemCaseSensitive(root, "active_preset_id");
  if (cJSON_IsString(j_active) && j_active->valuestring) {
    cfg.active_preset_id = j_active->valuestring;
  }

  const cJSON* j_presets = cJSON_GetObjectItemCaseSensitive(root, "presets");
  if (!cJSON_IsArray(j_presets) || cJSON_GetArraySize(j_presets) <= 0) {
    cJSON_Delete(root);
    if (err_detail) *err_detail = "presets must be a non-empty array";
    return ESP_ERR_INVALID_ARG;
  }

  const int n = cJSON_GetArraySize(j_presets);
  cfg.presets.reserve((size_t)n);
  std::set<std::string> ids;

  for (int i = 0; i < n; i++) {
    const cJSON* item = cJSON_GetArrayItem(j_presets, i);
    TimerPreset preset;
    std::string err;
    if (!parse_preset(item, &preset, &err)) {
      cJSON_Delete(root);
      if (err_detail) *err_detail = err;
      return ESP_ERR_INVALID_ARG;
    }

    if (ids.find(preset.id) != ids.end()) {
      cJSON_Delete(root);
      if (err_detail) *err_detail = "duplicate preset id: " + preset.id;
      return ESP_ERR_INVALID_ARG;
    }

    ids.insert(preset.id);
    cfg.presets.push_back(preset);
  }

  normalize_active_preset(&cfg);
  *out_cfg = cfg;
  cJSON_Delete(root);
  return ESP_OK;
}

std::string preset_store_to_json(const TimerConfig& cfg) {
  cJSON* root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "version", (double)cfg.version);
  cJSON_AddStringToObject(root, "active_preset_id", cfg.active_preset_id.c_str());

  cJSON* j_presets = cJSON_AddArrayToObject(root, "presets");
  for (const auto& preset : cfg.presets) {
    cJSON* j_preset = cJSON_CreateObject();
    cJSON_AddStringToObject(j_preset, "id", preset.id.c_str());
    cJSON_AddStringToObject(j_preset, "name", preset.name.c_str());

    cJSON* j_steps = cJSON_AddArrayToObject(j_preset, "steps");
    for (const auto& step : preset.steps) {
      cJSON* j_step = cJSON_CreateObject();
      cJSON_AddStringToObject(j_step, "name", step.name.c_str());
      cJSON_AddNumberToObject(j_step, "seconds", (double)step.seconds);
      cJSON_AddItemToArray(j_steps, j_step);
    }

    cJSON_AddItemToArray(j_presets, j_preset);
  }

  char* txt = cJSON_PrintUnformatted(root);
  std::string out = txt ? txt : "{}";
  if (txt) cJSON_free(txt);
  cJSON_Delete(root);
  return out;
}

esp_err_t preset_store_save(const TimerConfig& cfg) {
  if (!s_spiffs_mounted) return ESP_ERR_INVALID_STATE;
  const std::string body = preset_store_to_json(cfg);
  if (!write_file(kConfigPath, body)) {
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t preset_store_load_or_seed(TimerConfig* out_cfg) {
  if (!out_cfg) return ESP_ERR_INVALID_ARG;
  if (!s_spiffs_mounted) return ESP_ERR_INVALID_STATE;

  std::string body;
  if (!read_file(kConfigPath, &body)) {
    TimerConfig cfg = build_default_config();
    (void)preset_store_save(cfg);
    *out_cfg = cfg;
    ESP_LOGI(TAG, "seeded default presets at %s", kConfigPath);
    return ESP_OK;
  }

  TimerConfig parsed;
  std::string err;
  const esp_err_t st = preset_store_parse_json(body.data(), body.size(), &parsed, &err);
  if (st != ESP_OK) {
    ESP_LOGW(TAG, "preset parse failed (%s), restoring defaults", err.c_str());
    TimerConfig cfg = build_default_config();
    (void)preset_store_save(cfg);
    *out_cfg = cfg;
    return ESP_OK;
  }

  *out_cfg = parsed;
  return ESP_OK;
}

const char* preset_store_path() {
  return kConfigPath;
}
