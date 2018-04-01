#include <internal/WeavePlatformInternal.h>
#include <ConfigurationManager.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Profiles/security/WeaveApplicationKeys.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles::Security::AppKeys;
using namespace ::nl::Weave::Profiles::DeviceDescription;
using namespace ::WeavePlatform::Internal;

namespace WeavePlatform {

namespace {

class GroupKeyStore : public ::nl::Weave::Profiles::Security::AppKeys::GroupKeyStoreBase
{
public:
    virtual WEAVE_ERROR RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key);
    virtual WEAVE_ERROR StoreGroupKey(const WeaveGroupKey & key);
    virtual WEAVE_ERROR DeleteGroupKey(uint32_t keyId);
    virtual WEAVE_ERROR DeleteGroupKeysOfAType(uint32_t keyType);
    virtual WEAVE_ERROR EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount);
    virtual WEAVE_ERROR Clear(void);
    virtual WEAVE_ERROR GetCurrentUTCTime(uint32_t& utcTime);

protected:
    virtual WEAVE_ERROR RetrieveLastUsedEpochKeyId(void);
    virtual WEAVE_ERROR StoreLastUsedEpochKeyId(void);
};

GroupKeyStore gGroupKeyStore;

const char gNVSNamespace_Weave[]             = "weave";
const char gNVSNamespace_WeaveCounters[]     = "weave-counters";
const char gNVSNamespace_WeaveGroupKeys[]    = "weave-grp-keys";

const char gNVSKeyName_DeviceId[]            = "device-id";
const char gNVSKeyName_SerialNum[]           = "serial-num";
const char gNVSKeyName_ManufacturingDate[]   = "mfg-date";
const char gNVSKeyName_PairingCode[]         = "pairing-code";
const char gNVSKeyName_FabricId[]            = "fabric-id";
const char gNVSKeyName_DeviceCert[]          = "device-cert";
const char gNVSKeyName_DevicePrivateKey[]    = "device-key";
const char gNVSKeyName_ServiceConfig[]       = "service-config";
const char gNVSKeyName_PairedAccountId[]     = "account-id";
const char gNVSKeyName_ServiceId[]           = "service-id";
const char gNVSKeyName_FabricSecret[]        = "fabric-secret";
const char gNVSKeyName_ServiceRootKey[]      = "srk";
const char gNVSKeyName_EpochKeyPrefix[]      = "ek-";
const char gNVSKeyName_AppMasterKeyIndex[]   = "amk-index";
const char gNVSKeyName_AppMasterKeyPrefix[]  = "amk-";
const char gNVSKeyName_LastUsedEpochKeyId[]  = "last-ek-id";

extern const uint64_t gTestDeviceId;
extern const uint8_t gTestDeviceCert[];
extern const uint16_t gTestDeviceCertLength;
extern const uint8_t gTestDevicePrivateKey[];
extern const uint16_t gTestDevicePrivateKeyLength;

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint8_t * buf, size_t bufSize, size_t & outLen);
WEAVE_ERROR GetNVS(const char * ns, const char * name, char * buf, size_t bufSize, size_t & outLen);
WEAVE_ERROR GetNVS(const char * ns, const char * name, uint32_t & val);
WEAVE_ERROR GetNVS(const char * ns, const char * name, uint64_t & val);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, const uint8_t * data, size_t dataLen);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, const char * data);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint32_t val);
WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint64_t val);
WEAVE_ERROR ClearNVSKey(const char * ns, const char * name);
WEAVE_ERROR ClearNVSNamespace(const char * ns);
WEAVE_ERROR GetNVSBlobLength(const char * ns, const char * name, size_t & outLen);
WEAVE_ERROR EnsureNamespace(const char * ns);

} // unnamed namespace


// ==================== Configuration Manager Public Methods ====================

WEAVE_ERROR ConfigurationManager::GetVendorId(uint16_t& vendorId)
{
    vendorId = (uint16_t)CONFIG_DEVICE_VENDOR_ID;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductId(uint16_t& productId)
{
    productId = (uint16_t)CONFIG_DEVICE_PRODUCT_ID;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetProductRevision(uint16_t& productRev)
{
    productRev = (uint16_t)CONFIG_DEVICE_PRODUCT_REVISION;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ConfigurationManager::GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_SerialNum, buf, bufSize, serialNumLen);
}

WEAVE_ERROR ConfigurationManager::GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
{
    WEAVE_ERROR err;
    char dateStr[11]; // sized for big-endian date: YYYY-MM-DD
    size_t dateLen;
    char *parseEnd;

    err = GetNVS(gNVSNamespace_Weave, gNVSKeyName_ManufacturingDate, dateStr, sizeof(dateStr), dateLen);
    SuccessOrExit(err);

    VerifyOrExit(dateLen == sizeof(dateStr), err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(dateStr, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 4, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = strtoul(dateStr + 5, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 7, err = WEAVE_ERROR_INVALID_ARGUMENT);

    dayOfMonth = strtoul(dateStr + 8, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 10, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
{
    // TODO: get from build config
    outLen = 0;
    return WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
}

WEAVE_ERROR ConfigurationManager::GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
        uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * buildDateStr = __DATE__; // e.g. Feb 12 1996
    const char * buildTimeStr = __TIME__; // e.g. 23:59:01
    char monthStr[4];
    char * p;

    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    memcpy(monthStr, buildDateStr, 3);
    monthStr[3] = 0;

    p = strstr(months, monthStr);
    VerifyOrExit(p != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = ((p - months) / 3) + 1;

    dayOfMonth = strtoul(buildDateStr + 4, &p, 10);
    VerifyOrExit(p == buildDateStr + 6, err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(buildDateStr + 7, &p, 10);
    VerifyOrExit(p == buildDateStr + 11, err = WEAVE_ERROR_INVALID_ARGUMENT);

    hour = strtoul(buildTimeStr, &p, 10);
    VerifyOrExit(p == buildTimeStr + 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    minute = strtoul(buildTimeStr + 3, &p, 10);
    VerifyOrExit(p == buildTimeStr + 5, err = WEAVE_ERROR_INVALID_ARGUMENT);

    second = strtoul(buildTimeStr + 6, &p, 10);
    VerifyOrExit(p == buildTimeStr + 8, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDeviceCertificate(uint8_t * buf, size_t bufSize, size_t & certLen)
{
    WEAVE_ERROR err;

    err = GetNVS(gNVSNamespace_Weave, gNVSKeyName_DeviceCert, buf, bufSize, certLen);

    // TODO: make this a debug-only feature
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        certLen = gTestDeviceCertLength;
        VerifyOrExit(gTestDeviceCertLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        ESP_LOGI(TAG, "Device certificate not found in nvs; using default");
        memcpy(buf, gTestDeviceCert, gTestDeviceCertLength);
        err = WEAVE_NO_ERROR;
    }

    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDeviceCertificateLength(size_t & certLen)
{
    WEAVE_ERROR err = GetDeviceCertificate((uint8_t *)NULL, 0, certLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDevicePrivateKey(uint8_t * buf, size_t bufSize, size_t & keyLen)
{
    WEAVE_ERROR err;

    err = GetNVS(gNVSNamespace_Weave, gNVSKeyName_DevicePrivateKey, buf, bufSize, keyLen);

    // TODO: make this a debug-only feature
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        keyLen = gTestDevicePrivateKeyLength;
        VerifyOrExit(gTestDevicePrivateKeyLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        ESP_LOGI(TAG, "Device private key not found in nvs; using default");
        memcpy(buf, gTestDevicePrivateKey, gTestDevicePrivateKeyLength);
        err = WEAVE_NO_ERROR;
    }

    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDevicePrivateKeyLength(size_t & keyLen)
{
    WEAVE_ERROR err = GetDevicePrivateKey((uint8_t *)NULL, 0, keyLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::GetServiceConfig(uint8_t * buf, size_t bufSize, size_t & serviceConfigLen)
{
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_ServiceConfig, buf, bufSize, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManager::GetServiceConfigLength(size_t & serviceConfigLen)
{
    WEAVE_ERROR err = GetServiceConfig((uint8_t *)NULL, 0, serviceConfigLen);
    if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::GetServiceId(uint64_t & serviceId)
{
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_ServiceId, serviceId);
}

WEAVE_ERROR ConfigurationManager::GetPairedAccountId(char * buf, size_t bufSize, size_t accountIdLen)
{
    return GetNVS(gNVSNamespace_Weave, gNVSKeyName_PairedAccountId, buf, bufSize, accountIdLen);
}

WEAVE_ERROR ConfigurationManager::StoreDeviceId(uint64_t deviceId)
{
    return (deviceId != kNodeIdNotSpecified)
           ? StoreNVS(gNVSNamespace_Weave, gNVSKeyName_DeviceId, deviceId)
           : ClearNVSKey(gNVSNamespace_Weave, gNVSKeyName_DeviceId);
}

WEAVE_ERROR ConfigurationManager::StoreSerialNumber(const char * serialNum)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_SerialNum, serialNum);
}

WEAVE_ERROR ConfigurationManager::StoreManufacturingDate(const char * mfgDate)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_ManufacturingDate, mfgDate);
}

WEAVE_ERROR ConfigurationManager::StoreFabricId(uint64_t fabricId)
{
    return (fabricId != kFabricIdNotSpecified)
           ? StoreNVS(gNVSNamespace_Weave, gNVSKeyName_FabricId, fabricId)
           : ClearNVSKey(gNVSNamespace_Weave, gNVSKeyName_FabricId);
}

WEAVE_ERROR ConfigurationManager::StoreDeviceCertificate(const uint8_t * cert, size_t certLen)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_DeviceCert, cert, certLen);
}

WEAVE_ERROR ConfigurationManager::StoreDevicePrivateKey(const uint8_t * key, size_t keyLen)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_DevicePrivateKey, key, keyLen);
}

WEAVE_ERROR ConfigurationManager::StorePairingCode(const char * pairingCode)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_PairingCode, pairingCode);
}

WEAVE_ERROR ConfigurationManager::StoreServiceProvisioningData(uint64_t serviceId,
        const uint8_t * serviceConfig, size_t serviceConfigLen,
        const char * accountId, size_t accountIdLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;
    char *accountIdCopy = NULL;

    err = nvs_open(gNVSNamespace_Weave, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u64(handle, gNVSKeyName_ServiceId, serviceId);
    SuccessOrExit(err);

    err = nvs_set_blob(handle, gNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
    SuccessOrExit(err);

    accountIdCopy = strndup(accountId, accountIdLen);
    VerifyOrExit(accountIdCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = nvs_set_str(handle, gNVSKeyName_PairedAccountId, accountIdCopy);
    free(accountIdCopy);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::ClearServiceProvisioningData()
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(gNVSNamespace_Weave, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_key(handle, gNVSKeyName_ServiceId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = nvs_erase_key(handle, gNVSKeyName_ServiceConfig);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = nvs_erase_key(handle, gNVSKeyName_PairedAccountId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Commit to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::StoreServiceConfig(const uint8_t * serviceConfig, size_t serviceConfigLen)
{
    return StoreNVS(gNVSNamespace_Weave, gNVSKeyName_ServiceConfig, serviceConfig, serviceConfigLen);
}

WEAVE_ERROR ConfigurationManager::GetPersistedCounter(const char * key, uint32_t & value)
{
    WEAVE_ERROR err = GetNVS(gNVSNamespace_WeaveCounters, key, value);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }
    return err;
}

WEAVE_ERROR ConfigurationManager::StorePersistedCounter(const char * key, uint32_t value)
{
    return StoreNVS(gNVSNamespace_WeaveCounters, key, value);
}

WEAVE_ERROR ConfigurationManager::GetDeviceDescriptor(WeaveDeviceDescriptor & deviceDesc)
{
    WEAVE_ERROR err;
    size_t outLen;

    deviceDesc.Clear();

    deviceDesc.DeviceId = FabricState.LocalNodeId;

    deviceDesc.FabricId = FabricState.FabricId;

    err = GetVendorId(deviceDesc.VendorId);
    SuccessOrExit(err);

    err = GetProductId(deviceDesc.ProductId);
    SuccessOrExit(err);

    err = GetProductRevision(deviceDesc.ProductRevision);
    SuccessOrExit(err);

    err = GetManufacturingDate(deviceDesc.ManufacturingDate.Year, deviceDesc.ManufacturingDate.Month, deviceDesc.ManufacturingDate.Day);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // TODO: return PrimaryWiFiMACAddress

    err = GetSerialNumber(deviceDesc.SerialNumber, sizeof(deviceDesc.SerialNumber), outLen);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    err = GetFirmwareRevision(deviceDesc.SoftwareVersion, sizeof(deviceDesc.SoftwareVersion), outLen);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::GetDeviceDescriptorTLV(uint8_t * buf, size_t bufSize, size_t & encodedLen)
{
    WEAVE_ERROR err;
    WeaveDeviceDescriptor deviceDesc;

    err = GetDeviceDescriptor(deviceDesc);
    SuccessOrExit(err);

    err = WeaveDeviceDescriptor::EncodeTLV(deviceDesc, buf, (uint32_t)bufSize, encodedLen);
    SuccessOrExit(err);

exit:
    return err;
}

bool ConfigurationManager::IsServiceProvisioned()
{
    WEAVE_ERROR err;
    uint64_t serviceId;

    err = GetServiceId(serviceId);
    return (err == WEAVE_NO_ERROR && serviceId != 0);
}


// ==================== Configuration Manager Private Methods ====================

WEAVE_ERROR ConfigurationManager::Init()
{
    WEAVE_ERROR err;

    // Force initialization of weave NVS namespaces if they doesn't already exist.
    err = EnsureNamespace(gNVSNamespace_Weave);
    SuccessOrExit(err);
    err = EnsureNamespace(gNVSNamespace_WeaveCounters);
    SuccessOrExit(err);
    err = EnsureNamespace(gNVSNamespace_WeaveGroupKeys);
    SuccessOrExit(err);

    // Force initialization of the global GroupKeyStore object.
    new ((void *)&gGroupKeyStore) GroupKeyStore();

exit:
    return err;
}

WEAVE_ERROR ConfigurationManager::ConfigureWeaveStack()
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;
    size_t pairingCodeLen;

    err = nvs_open(gNVSNamespace_Weave, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    // Read the device id from NVS.
    err = nvs_get_u64(handle, gNVSKeyName_DeviceId, &FabricState.LocalNodeId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // TODO: make this a DEBUG-only feature
        ESP_LOGI(TAG, "Device id not found in nvs; using default: %" PRIX64, gTestDeviceId);
        FabricState.LocalNodeId = gTestDeviceId;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Read the fabric id from NVS.  If not present, then the device is not currently a
    // member of a Weave fabric.
    err = nvs_get_u64(handle, gNVSKeyName_FabricId, &FabricState.FabricId);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        FabricState.FabricId = kFabricIdNotSpecified;
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Read the pairing code from NVS.
    pairingCodeLen = sizeof(mPairingCode);
    err = nvs_get_str(handle, gNVSKeyName_PairingCode, mPairingCode, &pairingCodeLen);
    if (err == ESP_ERR_NVS_NOT_FOUND || pairingCodeLen == 0)
    {
        // TODO: make this a DEBUG-only feature
        ESP_LOGI(TAG, "Pairing code not found in nvs; using default: %s", CONFIG_DEFAULT_PAIRING_CODE);
        strcpy(mPairingCode, CONFIG_DEFAULT_PAIRING_CODE);
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    FabricState.PairingCode = mPairingCode;

    // Configure the FabricState object with a reference to the GroupKeyStore object.
    FabricState.GroupKeyStore = &gGroupKeyStore;

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}


namespace {

// ==================== Group Key Store Implementation ====================

WEAVE_ERROR GroupKeyStore::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey & key)
{
    WEAVE_ERROR err;
    size_t keyLen;

    // TODO: add support for other group key types

    VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_KEY_NOT_FOUND);

    err = GetNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret, key.Key, sizeof(key.Key), keyLen);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_ERROR_KEY_NOT_FOUND;
    }
    SuccessOrExit(err);

    key.KeyId = keyId;
    key.KeyLen = keyLen;

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::StoreGroupKey(const WeaveGroupKey & key)
{
    WEAVE_ERROR err;

    // TODO: add support for other group key types

    VerifyOrExit(key.KeyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_INVALID_KEY_ID);

    err = StoreNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret, key.Key, key.KeyLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKey(uint32_t keyId)
{
    WEAVE_ERROR err;

    // TODO: add support for other group key types

    VerifyOrExit(keyId == WeaveKeyId::kFabricSecret, err = WEAVE_ERROR_KEY_NOT_FOUND);

    err = ClearNVSKey(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::DeleteGroupKeysOfAType(uint32_t keyType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // TODO: add support for other group key types

    if (WeaveKeyId::IsGeneralKey(keyType))
    {
        err = ClearNVSKey(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::EnumerateGroupKeys(uint32_t keyType, uint32_t * keyIds,
        uint8_t keyIdsArraySize, uint8_t & keyCount)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t keyLen;

    // Verify the supported key type is specified.
    VerifyOrExit(WeaveKeyId::IsGeneralKey(keyType) ||
                 WeaveKeyId::IsAppRootKey(keyType) ||
                 WeaveKeyId::IsAppEpochKey(keyType) ||
                 WeaveKeyId::IsAppGroupMasterKey(keyType), err = WEAVE_ERROR_INVALID_KEY_ID);

    keyCount = 0;

    if (WeaveKeyId::IsGeneralKey(keyType))
    {
        err = GetNVSBlobLength(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_FabricSecret, keyLen);
        SuccessOrExit(err);

        if (keyLen != 0)
        {
            VerifyOrExit(keyCount < keyIdsArraySize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            keyIds[keyCount++] = WeaveKeyId::kFabricSecret;
        }
    }

    // TODO: add support for other group key types

exit:
    return err;
}

WEAVE_ERROR GroupKeyStore::Clear(void)
{
    return ClearNVSNamespace(gNVSNamespace_WeaveGroupKeys);
}

WEAVE_ERROR GroupKeyStore::GetCurrentUTCTime(uint32_t & utcTime)
{
    // TODO: support real time when available.
    return WEAVE_ERROR_UNSUPPORTED_CLOCK;
}

WEAVE_ERROR GroupKeyStore::RetrieveLastUsedEpochKeyId(void)
{
    WEAVE_ERROR err;

    err = GetNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
    if (err == WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND)
    {
        LastUsedEpochKeyId = WeaveKeyId::kNone;
        err = WEAVE_NO_ERROR;
    }
    return err;
}

WEAVE_ERROR GroupKeyStore::StoreLastUsedEpochKeyId(void)
{
    return StoreNVS(gNVSNamespace_WeaveGroupKeys, gNVSKeyName_LastUsedEpochKeyId, LastUsedEpochKeyId);
}

// ==================== Utility Functions for accessing ESP NVS ====================

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = bufSize;
    err = nvs_get_blob(handle, name, buf, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR GetNVS(const char * ns, const char * name, char * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = bufSize;
    err = nvs_get_str(handle, name, buf, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

    outLen -= 1; // Don't count trailing nul.

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint32_t & val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_get_u32(handle, name, &val);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR GetNVS(const char * ns, const char * name, uint64_t & val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_get_u64(handle, name, &val);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_PLATFORM_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, const uint8_t * data, size_t dataLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    if (data != NULL)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_set_blob(handle, name, data, dataLen);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);
    }

    else
    {
        err = ClearNVSKey(ns, name);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, const char * data)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    if (data != NULL)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_set_str(handle, name, data);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);
    }

    else
    {
        err = ClearNVSKey(ns, name);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint32_t val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u32(handle, name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR StoreNVS(const char * ns, const char * name, uint64_t val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u64(handle, name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ClearNVSKey(const char * ns, const char * name)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_key(handle, name);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ClearNVSNamespace(const char * ns)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_all(handle);
    SuccessOrExit(err);

    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR GetNVSBlobLength(const char * ns, const char * name, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = 0;
    err = nvs_get_blob(handle, name, NULL, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        ExitNow(err = WEAVE_NO_ERROR);
    }
    if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR EnsureNamespace(const char * ns)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_commit(handle);
        SuccessOrExit(err);
    }
    SuccessOrExit(err);
    needClose = true;

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

// ==================== Test Device Credentials ====================

const uint64_t gTestDeviceId = 0x18B4300000000001ULL;

const uint8_t gTestDeviceCert[] =
{
    /*
    -----BEGIN CERTIFICATE-----
    MIIBhzCCAT6gAwIBAgIIEEjK8u1PmzAwCQYHKoZIzj0EATAiMSAwHgYKKwYBBAGC
    wysBAwwQMThCNDMwRUVFRTAwMDAwMjAeFw0xMzEwMjIwMDQ3MDBaFw0yMzEwMjAw
    MDQ3MDBaMCIxIDAeBgorBgEEAYLDKwEBDBAxOEI0MzAwMDAwMDAwMDAxME4wEAYH
    KoZIzj0CAQYFK4EEACEDOgAE72edUwyZ/51yQrH5tmAgjiWfNXLwo+eD5lYUk/lo
    RWWLJDFeh4xkNSWHGQOZzUWhJPp2CxKeOX6jajBoMAwGA1UdEwEB/wQCMAAwDgYD
    VR0PAQH/BAQDAgWgMCAGA1UdJQEB/wQWMBQGCCsGAQUFBwMCBggrBgEFBQcDATAR
    BgNVHQ4ECgQITv9HUeTGY5swEwYDVR0jBAwwCoAIRONAOKnUtacwCQYHKoZIzj0E
    AQM4ADA1Ahhdt1KwlRMRcfFbZAOAjBi+oSDxhrpFbBQCGQDFDc8mAoARjFE6vZWV
    dpR3yUb/7cCgPb0=
    -----END CERTIFICATE-----
    */

    0xd5, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x30, 0x01, 0x08, 0x10, 0x48, 0xca, 0xf2, 0xed, 0x4f,
    0x9b, 0x30, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00, 0x27, 0x13, 0x02, 0x00, 0x00, 0xee, 0xee, 0x30,
    0xb4, 0x18, 0x18, 0x26, 0x04, 0x04, 0x23, 0x73, 0x1a, 0x26, 0x05, 0x04, 0xcc, 0x98, 0x2d, 0x57,
    0x06, 0x00, 0x27, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x30, 0xb4, 0x18, 0x18, 0x24, 0x07, 0x02,
    0x24, 0x08, 0x25, 0x30, 0x0a, 0x39, 0x04, 0xef, 0x67, 0x9d, 0x53, 0x0c, 0x99, 0xff, 0x9d, 0x72,
    0x42, 0xb1, 0xf9, 0xb6, 0x60, 0x20, 0x8e, 0x25, 0x9f, 0x35, 0x72, 0xf0, 0xa3, 0xe7, 0x83, 0xe6,
    0x56, 0x14, 0x93, 0xf9, 0x68, 0x45, 0x65, 0x8b, 0x24, 0x31, 0x5e, 0x87, 0x8c, 0x64, 0x35, 0x25,
    0x87, 0x19, 0x03, 0x99, 0xcd, 0x45, 0xa1, 0x24, 0xfa, 0x76, 0x0b, 0x12, 0x9e, 0x39, 0x7e, 0x35,
    0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x05, 0x18, 0x35, 0x84, 0x29, 0x01,
    0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x18, 0x18, 0x35, 0x81, 0x30, 0x02, 0x08, 0x4e, 0xff, 0x47,
    0x51, 0xe4, 0xc6, 0x63, 0x9b, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08, 0x44, 0xe3, 0x40, 0x38, 0xa9,
    0xd4, 0xb5, 0xa7, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x18, 0x5d, 0xb7, 0x52, 0xb0, 0x95, 0x13, 0x11,
    0x71, 0xf1, 0x5b, 0x64, 0x03, 0x80, 0x8c, 0x18, 0xbe, 0xa1, 0x20, 0xf1, 0x86, 0xba, 0x45, 0x6c,
    0x14, 0x30, 0x02, 0x19, 0x00, 0xc5, 0x0d, 0xcf, 0x26, 0x02, 0x80, 0x11, 0x8c, 0x51, 0x3a, 0xbd,
    0x95, 0x95, 0x76, 0x94, 0x77, 0xc9, 0x46, 0xff, 0xed, 0xc0, 0xa0, 0x3d, 0xbd, 0x18, 0x18
};

const uint16_t gTestDeviceCertLength = sizeof(gTestDeviceCert);

const uint8_t gTestDevicePrivateKey[] =
{

    /*
    -----BEGIN EC PRIVATE KEY-----
    MGgCAQEEHKmH22bJe2mLomgimLLt70dguW98M6EsKajVuO6gBwYFK4EEACGhPAM6
    AATvZ51TDJn/nXJCsfm2YCCOJZ81cvCj54PmVhST+WhFZYskMV6HjGQ1JYcZA5nN
    RaEk+nYLEp45fg==
    -----END EC PRIVATE KEY-----
    */

    0xd5, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x24, 0x01, 0x25, 0x30, 0x02, 0x1c, 0xa9, 0x87, 0xdb,
    0x66, 0xc9, 0x7b, 0x69, 0x8b, 0xa2, 0x68, 0x22, 0x98, 0xb2, 0xed, 0xef, 0x47, 0x60, 0xb9, 0x6f,
    0x7c, 0x33, 0xa1, 0x2c, 0x29, 0xa8, 0xd5, 0xb8, 0xee, 0x30, 0x03, 0x39, 0x04, 0xef, 0x67, 0x9d,
    0x53, 0x0c, 0x99, 0xff, 0x9d, 0x72, 0x42, 0xb1, 0xf9, 0xb6, 0x60, 0x20, 0x8e, 0x25, 0x9f, 0x35,
    0x72, 0xf0, 0xa3, 0xe7, 0x83, 0xe6, 0x56, 0x14, 0x93, 0xf9, 0x68, 0x45, 0x65, 0x8b, 0x24, 0x31,
    0x5e, 0x87, 0x8c, 0x64, 0x35, 0x25, 0x87, 0x19, 0x03, 0x99, 0xcd, 0x45, 0xa1, 0x24, 0xfa, 0x76,
    0x0b, 0x12, 0x9e, 0x39, 0x7e, 0x18
};

const uint16_t gTestDevicePrivateKeyLength = sizeof(gTestDevicePrivateKey);

} // unnamed namespace
} // namespace WeavePlatform

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

using namespace ::WeavePlatform;

WEAVE_ERROR Read(Key key, uint32_t & value)
{
    return ConfigurationMgr.GetPersistedCounter(key, value);
}

WEAVE_ERROR Write(Key key, uint32_t value)
{
    return ConfigurationMgr.StorePersistedCounter(key, value);
}

} // PersistedStorage
} // Platform
} // Weave
} // nl
