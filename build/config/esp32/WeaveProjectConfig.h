/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      Weave project configuration for the ESP32 platform.
 *
 */
#ifndef WEAVEPROJECTCONFIG_H
#define WEAVEPROJECTCONFIG_H

#include "esp_err.h"


// NOTE: The values that are mapped to CONFIG_ #defines are settable via the ESP-IDF Kconfig mechanism.

// ==================== General Configuration ====================

#define WEAVE_CONFIG_MAX_BINDINGS CONFIG_MAX_BINDINGS
#define WEAVE_CONFIG_MAX_CONNECTIONS CONFIG_MAX_CONNECTIONS
#define WEAVE_CONFIG_MAX_PEER_NODES CONFIG_MAX_PEER_NODES
#define WEAVE_CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS CONFIG_MAX_UNSOLICITED_MESSAGE_HANDLERS
#define WEAVE_CONFIG_MAX_EXCHANGE_CONTEXTS CONFIG_MAX_EXCHANGE_CONTEXTS
#define WEAVE_CONFIG_SHORT_ERROR_STR CONFIG_SHORT_ERROR_STR
#define WEAVE_LOG_FILTERING 0

// ==================== Security Configuration ====================

#define WEAVE_CONFIG_MAX_SESSION_KEYS CONFIG_MAX_SESSION_KEYS
#define WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
#define WEAVE_CONFIG_MAX_CACHED_MSG_ENC_APP_KEYS CONFIG_MAX_CACHED_MSG_ENC_APP_KEYS
#define WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS CONFIG_MAX_APPLICATION_EPOCH_KEYS
#define WEAVE_CONFIG_MAX_APPLICATION_GROUPS CONFIG_MAX_APPLICATION_GROUPS
#define WEAVE_CONFIG_DEFAULT_SECURITY_SESSION_ESTABLISHMENT_TIMEOUT CONFIG_DEFAULT_SECURITY_SESSION_ESTABLISHMENT_TIMEOUT
#define WEAVE_CONFIG_DEFAULT_SECURITY_SESSION_IDLE_TIMEOUT CONFIG_DEFAULT_SECURITY_SESSION_IDLE_TIMEOUT
#define WEAVE_CONFIG_SECURITY_TEST_MODE CONFIG_SECURITY_TEST_MODE
#define WEAVE_CONFIG_DEBUG_CERT_VALIDATION CONFIG_DEBUG_CERT_VALIDATION
#define WEAVE_CONFIG_ENABLE_PASE_INITIATOR CONFIG_ENABLE_PASE_INITIATOR
#define WEAVE_CONFIG_ENABLE_PASE_RESPONDER CONFIG_ENABLE_PASE_RESPONDER
#define WEAVE_CONFIG_ENABLE_CASE_INITIATOR CONFIG_ENABLE_CASE_INITIATOR
#define WEAVE_CONFIG_ENABLE_CASE_RESPONDER CONFIG_ENABLE_CASE_RESPONDER
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG0 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG1 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG2 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG3 0
#define WEAVE_CONFIG_SUPPORT_PASE_CONFIG4 1
#define WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR 0

// ==================== Platform Adaptations ====================

#define WEAVE_CONFIG_MAX_INTERFACES 4
#define WEAVE_CONFIG_MAX_LOCAL_ADDR_UDP_ENDPOINTS 4
#define WEAVE_CONFIG_MAX_TUNNELS 0
#define WEAVE_CONFIG_ENABLE_TUNNELING 1
#define WEAVE_CONFIG_PERSISTED_STORAGE_ENC_MSG_CNTR_ID "enc-msg-counter"
// The ESP NVS implementation limits key names to 15 characters.
#define WEAVE_CONFIG_PERSISTED_STORAGE_MAX_KEY_LENGTH 15

#define WEAVE_CONFIG_ERROR_TYPE esp_err_t
#define WEAVE_CONFIG_NO_ERROR ESP_OK
#define WEAVE_CONFIG_ERROR_MIN 4000000
#define WEAVE_CONFIG_ERROR_MAX 4000999

#define ASN1_CONFIG_ERROR_TYPE esp_err_t
#define ASN1_CONFIG_NO_ERROR ESP_OK
#define ASN1_CONFIG_ERROR_MIN 5000000
#define ASN1_CONFIG_ERROR_MAX 5000999

#define WeaveDie() abort()

// ==================== Security Adaptations ====================

#define WEAVE_CONFIG_USE_OPENSSL_ECC 0
#define WEAVE_CONFIG_USE_MICRO_ECC 1
#define WEAVE_CONFIG_HASH_IMPLEMENTATION_OPENSSL 0
#define WEAVE_CONFIG_HASH_IMPLEMENTATION_MINCRYPT 1
#define WEAVE_CONFIG_RNG_IMPLEMENTATION_OPENSSL 0
#define WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG 1
#define WEAVE_CONFIG_AES_IMPLEMENTATION_OPENSSL 0
#define WEAVE_CONFIG_AES_IMPLEMENTATION_AESNI 0
#define WEAVE_CONFIG_AES_IMPLEMENTATION_PLATFORM 1
#define WEAVE_CONFIG_ENABLE_PROVISIONING_BUNDLE_SUPPORT 0

#endif /* WEAVEPROJECTCONFIG_H */
