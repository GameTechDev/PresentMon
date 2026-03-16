#ifndef _UCI_DEFINES_H_
#define _UCI_DEFINES_H_

#include <stdint.h>

typedef struct _uciGUID {
    uint32_t dword;
    uint16_t word0;
    uint16_t word1;
    uint16_t word2;
    uint64_t qword;
} uciGUID;

#define UCI_DEFINE_GUID(name, dw, w0, w1, w2, qw) \
    uciGUID (name) = { (dw), (w0), (w1), (w2), (qw) };

#define ARE_UCI_GUIDS_EQUAL(GUID0, GUID1) \
    ((GUID0.dword == GUID1.dword) && \
     (GUID0.word0 == GUID1.word0) && \
     (GUID0.word1 == GUID1.word1) && \
     (GUID0.word2 == GUID1.word2) && \
     (GUID0.qword == GUID1.qword))

typedef struct _uciVersion {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t other;
} uciVersion;

#define UCI_DEFINE_VERSION(name, major, minor, patch, other) \
    uciVersion (name) = { (major), (minor), (patch), 0x0 };

#define GET_UCI_VERSION_UINT32(version) \
    (((uint32_t)(version).major << 24) | \
     ((uint32_t)(version).minor << 16) | \
     ((uint32_t)(version).patch << 8) | \
     ((uint32_t)(version).other))

#define ARE_UCI_VERSIONS_EQUAL(version0, version1) \
    ((version0.major == version1.major) && \
     (version0.minor == version1.minor) && \
     (version0.patch == version1.word1) && \
     (version0.other == version1.other))

#define ARE_UCI_VERSIONS_COMPATIBLE(userVersion, libVersion) \
    ((userVersion.major == libVersion.major) && \
    (GET_UCI_VERSION_UINT32(libVersion) >= GET_UCI_VERSION_UINT32(userVersion)))

typedef struct _uciIdentifier {
    char name[64];
    uciGUID guid;
    uciVersion version;
} uciIdentifier;

#define UCI_DEFINE_IDENTIFIER(name, strName, guid, version) \
    uciIdentifier (name) = { (strName), (guid), (version) };

#define GET_UCI_COLLECTOR_ID_DWORD(identifier) \
    ((identifier).guid.dword)

// Define the result type
typedef enum uc_result_t {
    UC_SUCCESS = 0,
    UC_ERROR_MEMORY_ALLOCATION_FAILED = 1,
    UC_ERROR_INVALID_PARAMETER = 2,
    UC_NO_DEVICES_FOUND = 3,
    UC_NO_METRIC_GROUPS_FOUND = 4,
    UC_NO_METRICS_FOUND = 5,
    UC_NO_EVENTS_FOUND = 6,
    UC_ERROR_INVALID_CONFIGURATION = 7,
    UC_ERROR_NO_CONFIGURATION = 8,
    UC_ERROR_COLLECTION_IS_RUNNING = 9,
    UC_ERROR_COLLECTION_NOT_RUNNING = 10,
    UC_ERROR_COLLECTION_ALREADY_PAUSED = 11,
    UC_ERROR_COLLECTION_NOT_PAUSED = 12,
    UC_ERROR_NO_DATA = 13,
    UC_ERROR_INVALID_FORMAT = 14,
    UC_ERROR_FILE_ACCESS = 15,
    UC_ERROR_NO_DRIVER_CONNECTION = 16,
    UC_ERROR_LIB_HANDLE_NOT_FOUND = 17,
    UC_ERROR_LIB_UNINITIALIZED = 18,
    UC_ERROR_VERSION_INCOMPATIBLE = 19,
    UC_ERROR_FUNCTION_NOT_SUPPORTED = 20,
    UC_ERROR_CALLBACK_NOT_SET = 21,
    UC_ERROR_NOT_ROOT = 22,
    UC_ERROR_PROCESS_LAUNCH_FAILED = 23,
    UC_ERROR_UNKNOWN = 0xFF
} uc_result_t;

#endif // _UCI_VERSIONS_H_
