// SPDX-License-Identifier: Apache-2.0
// The ESP32 side of signed updates (hub ADR 012): the Arduino `Update` class writes the other
// app slot, ESP-IDF app rollback keeps or drops the new image, NVS keeps three short strings,
// and mbedTLS checks the signature and the image hash. Arduino-ESP32 2.0.17 (mbedTLS 2.28);
// the calls used here also exist in 3.x (ESP-CORE-001).
//
// Rollback: the sketch must define `extern "C" bool verifyRollbackLater() { return true; }` so
// Arduino does not mark a new image valid at boot. The Updater marks it valid once Online.
#pragma once
#include "GrokUpdate.h"
#include <Preferences.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>

namespace grok {
class EspUpdatePlatform : public UpdatePlatform {
public:
  size_t slotSize() override {
    const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
    return next ? next->size : 0;
  }
  bool begin(size_t size) override { return Update.begin(size, U_FLASH); }
  bool write(const uint8_t *data, size_t size) override {
    return Update.write(const_cast<uint8_t *>(data), size) == size;
  }
  // `Update` writes the first bytes last, so a partial image never boots; end() checks the
  // image header and sets the boot slot, which ESP-IDF marks as new (pending verify).
  bool finish() override { return Update.end(true); }
  void abort() override {
    if (Update.isRunning())
      Update.abort();
  }
  bool pendingVerify() override {
    esp_ota_img_states_t state;
    return esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
           state == ESP_OTA_IMG_PENDING_VERIFY;
  }
  void markValid() override { esp_ota_mark_app_valid_cancel_rollback(); }
  void revert() override { esp_ota_mark_app_invalid_rollback_and_reboot(); }
  bool reverted() override { return esp_ota_get_last_invalid_partition() != nullptr; }
  void restart() override { ESP.restart(); }
  bool load(const char *key, char *value, size_t size) override {
    Preferences nvs;
    if (!nvs.begin("grok-update", true))
      return false;
    bool found = nvs.isKey(key) && nvs.getString(key, value, size) > 0;
    nvs.end();
    return found;
  }
  void store(const char *key, const char *value) override {
    Preferences nvs;
    if (nvs.begin("grok-update", false)) {
      nvs.putString(key, value);
      nvs.end();
    }
  }
};

// Up to two compiled PEM public keys, so a builder can move to a new key.
class MbedtlsUpdateCrypto : public UpdateCrypto {
public:
  explicit MbedtlsUpdateCrypto(const char *key, const char *second = nullptr) {
    const char *keys[2] = {key, second};
    for (const char *pem : keys) {
      if (!pem || count_ == 2)
        continue;
      mbedtls_pk_init(&pk_[count_]);
      // The PEM length must include the terminating NUL.
      if (mbedtls_pk_parse_public_key(&pk_[count_], reinterpret_cast<const unsigned char *>(pem),
                                      strlen(pem) + 1) == 0 &&
          mbedtls_pk_can_do(&pk_[count_], MBEDTLS_PK_ECDSA))
        ++count_;
      else
        mbedtls_pk_free(&pk_[count_]);
    }
    mbedtls_md_init(&md_);
  }
  ~MbedtlsUpdateCrypto() override {
    for (size_t i = 0; i < count_; ++i)
      mbedtls_pk_free(&pk_[i]);
    mbedtls_md_free(&md_);
  }
  size_t keys() const { return count_; }
  bool verify(const uint8_t *message, size_t size, const uint8_t *signature,
              size_t signatureSize) override {
    uint8_t hash[32];
    const mbedtls_md_info_t *sha = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!sha || mbedtls_md(sha, message, size, hash))
      return false;
    for (size_t i = 0; i < count_; ++i)
      if (mbedtls_pk_verify(&pk_[i], MBEDTLS_MD_SHA256, hash, sizeof(hash), signature,
                            signatureSize) == 0)
        return true;
    return false;
  }
  void hashStart() override {
    mbedtls_md_free(&md_);
    mbedtls_md_init(&md_);
    mbedtls_md_setup(&md_, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&md_);
  }
  void hashUpdate(const uint8_t *data, size_t size) override {
    mbedtls_md_update(&md_, data, size);
  }
  void hashFinish(uint8_t digest[32]) override { mbedtls_md_finish(&md_, digest); }

private:
  mbedtls_pk_context pk_[2];
  size_t count_ = 0;
  mbedtls_md_context_t md_;
};
} // namespace grok
