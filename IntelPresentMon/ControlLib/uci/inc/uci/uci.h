#ifndef UCI_H_
#define UCI_H_

#include <stdint.h>

#include <uci/uci-defines.h>
#include <uci/uci-metrics.h>
#include <uci/uci-data-records.h>
#include <uci/uci_export.h>

/* clang-format off */
#if defined(__cplusplus)
extern "C" {
#endif

/** Handle to a data collector instance */
typedef struct Collector* uciCollectorHandle;

/*******************************
 * Collector Factory Functions *
 *******************************/

/**
 * @brief Retrieves the number of available collectors in the system.
 *
 * This function queries the unified collector manager to determine how many
 * collector implementations are currently available for use. This count can
 * be used to allocate memory for subsequent calls to retrieve all collectors.
 *
 * @param[out] num_of_available_collectors Pointer to receive the count of available collectors.
 *
 * @return UC_SUCCESS if the count was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if num_of_available_collectors is NULL.
 */
uc_result_t UCI_EXPORT uciNumberOfAvailableCollectors(uint32_t* num_of_available_collectors);

/**
 * @brief Retrieves handles to all available collectors on the loadable system.
 *
 * This function populates an array with handles to all collector implementations
 * that are currently available. The array must be pre-allocated with sufficient
 * space based on the count returned by uciNumberOfAvailableCollectors().
 *
 * @param[in] num_of_available_collectors The number of collector handles the array can hold.
 * @param[out] collectors Pre-allocated array to receive collector handles.
 *
 * @return UC_SUCCESS if all collector handles were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collectors is NULL or num_of_available_collectors is 0.
 */
uc_result_t UCI_EXPORT uciGetAllCollectors(uint32_t num_of_available_collectors, uciCollectorHandle* collectors);

/**
 * @brief Retrieves a specific collector by its unique identifier.
 *
 * This function searches for and returns a handle to a collector that matches
 * the provided identifier. The identifier uniquely identifies a collector by
 * its name, GUID, and version information.
 *
 * @param[in] collector_identifier Unique identifier specifying which collector to retrieve.
 * @param[out] collector Pointer to receive the handle to the matching collector.
 *
 * @return UC_SUCCESS if the collector was found and handle retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 */
uc_result_t UCI_EXPORT uciGetCollectorFromIdentifier(uciIdentifier collector_identifier, uciCollectorHandle* collector);

/**
 * @brief Parses a configuration string to extract collector identifier information.
 *
 * This function parses a configuration string (JSON format) to extract collector
 * identifier information including name, GUID, and version. The configuration string
 * format should contain the necessary fields to populate the identifier structure.
 *
 * Expected JSON input format:
 * {
 *   "collectorIdentifier": {
 *     "name": "<collector_name>",
 *     "guid": "<XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX>",
 *     "version": "<major.minor.patch[.other]>"
 *   }
 * }
 *
 * @param[in] config Configuration string containing identifier information.
 * @param[out] identifier Pointer to receive the parsed identifier structure.
 *
 * @return UC_SUCCESS if the identifier was parsed successfully.
 * @return UC_ERROR_INVALID_FORMAT if the configuration string is malformed or missing required fields.
 *
 * @note GUID format: Accepts standard hyphenated hexadecimal format (case-insensitive).
 *       Example: "54B46B56-5439-4D29-8B07-15863D1F6EC6"
 *
 * @note Name field: Copied to a 64-character buffer. Names longer than 63 characters
 *       will be truncated to fit.
 */
uc_result_t UCI_EXPORT uciReadIdentifierFromConfig(const char* config, uciIdentifier* identifier);

/**
 * @brief Serializes a collector identifier to a configuration string format.
 *
 * This function converts a collector identifier structure into a configuration
 * string representation (JSON) and allocates memory for the resulting string.
 *
 * Expected JSON output format:
 * {
 *   "collectorIdentifier": {
 *     "name": "<collector_name>",
 *     "guid": "<XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX>",
 *     "version": "<major.minor.patch[.other]>"
 *   }
 * }
 *
 * @param[in] identifier The identifier structure to serialize.
 * @param[out] config Pointer to receive the allocated configuration string.
 *                   The caller must free the returned string.
 *
 * @return UC_SUCCESS if the identifier was serialized successfully.
 * @return UC_ERROR_UNKNOWN if serialization failed due to an internal error.
 *
 * @note GUID format: The GUID string uses standard hyphenated hexadecimal format
 *       with uppercase letters. Example: "54B46B56-5439-4D29-8B07-15863D1F6EC6"
 *
 * @note Memory management: On success, a new string is allocated and returned via the
 *       config parameter. The caller is responsible for freeing this memory.
 *       On failure, the config parameter is not modified.
 */
uc_result_t UCI_EXPORT uciWriteIdentifierToConfig(uciIdentifier identifier, char** config);

/*********************************
 * Collector Interface Functions *
 *********************************/

/**
 * @brief Retrieves the name of the specified collector.
 *
 * This function returns a pointer to the collector's name string. The name
 * is a human-readable identifier that describes the collector implementation.
 *
 * @param[in] collector Handle to the collector whose name should be retrieved.
 * @param[out] name Pointer to receive the collector's name string.
 *
 * @return UC_SUCCESS if the name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector or name is NULL.
 *
 * @note The returned string is owned by the collector and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetCollectorName(uciCollectorHandle collector, const char* name[]);

/**
 * @brief Retrieves the unique GUID of the specified collector.
 *
 * This function returns the globally unique identifier (GUID) that uniquely
 * identifies this collector implementation across all systems and environments.
 *
 * @param[in] collector Handle to the collector whose GUID should be retrieved.
 * @param[out] guid Pointer to receive the collector's GUID structure.
 *
 * @return UC_SUCCESS if the GUID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector or guid is NULL.
 */
uc_result_t UCI_EXPORT uciGetCollectorGUID(uciCollectorHandle collector, uciGUID* guid);

/**
 * @brief Retrieves the version information of the specified collector.
 *
 * This function returns version information including major, minor, patch,
 * and other version components that identify the specific release of the collector.
 *
 * @param[in] collector Handle to the collector whose version should be retrieved.
 * @param[out] version Pointer to receive the collector's version structure.
 *
 * @return UC_SUCCESS if the version was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector or version is NULL.
 */
uc_result_t UCI_EXPORT uciGetCollectorVersion(uciCollectorHandle collector, uciVersion* version);

/**
 * @brief Initializes the specified collector for use.
 *
 * This function performs necessary initialization of the collector, preparing
 * it for device enumeration and metric collection operations. This must be
 * called before using any calling any other collection API.
 *
 * @param[in] collector handle to the collector to initialize.
 *
 * @return UC_SUCCESS if initialization completed successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 */
uc_result_t UCI_EXPORT uciInitialize(uciCollectorHandle collector);

/**
 * @brief Enumerates all metrics available for a specific device.
 *
 * This function queries and returns information about available metric groups,
 * individual metrics, and/or events that can be collected.
 * The metric container will be populated with a hierarchical representation of
 * available metrics.
 *
 * @param[in] collector Handle to the collector to use for metric enumeration.
 * @param[out] metric_container Pointer to a metric container handle that will be filled with metric information.
 *
 * @return UC_SUCCESS if metrics were enumerated successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector, device, or metric_container is NULL.
 * @return UC_ERROR_MEMORY_ALLOCATION_FAILED if memory allocation failed.
 */
uc_result_t UCI_EXPORT uciEnumerateMetrics(uciCollectorHandle collector, uciMetricContainerHandle* metric_container);

/**
 * @brief Configures metric collection based on a JSON configuration string.
 *
 * This function configures which metrics should be collected and how collection
 * should be performed. The configuration is specified as a JSON string containing
 * global collection parameters as well as device-specific configuration.
 *
 * Example JSON configuration format:
 * {
 *   "collectorIdentifier": {
 *     "name": "<collector_name>",
 *     "guid": "<XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX>",
 *     "version": "<major.minor.patch[.other]>"
 *   },
 *   "collectionParams": {
 *     "samplingInterval": <milliseconds>,
 *     "bufferSize": <bytes>,
 *     "enableTimestamps": <boolean>,
 *     ...
 *   },
 *   "metrics": [
 *     "<metric_name>",
 *     ...
 *   ],
 *   "outputParams": {
 *     "outputFormats": [
 *       ...
 *     ],
 *     ...
 *   }
 * }
 *
 * @param[in] collector Handle to the collector to configure.
 * @param[in] config_json JSON string containing the collection configuration.
 *
 * @return UC_SUCCESS if configuration was successful.
 * @return UC_ERROR_INVALID_PARAMETER if collector or config_json is NULL.
 * @return UC_ERROR_INVALID_CONFIGURATION if the configuration contains invalid JSON or values.
 * @return UC_NO_DEVICES_FOUND if no matching devices were found for configuration.
 */
uc_result_t UCI_EXPORT uciConfigureCollection(uciCollectorHandle collector, const char* config_json);

/**
 * @brief Sets a callback function to receive metric data during collection.
 *
 * This function registers a callback that will be invoked whenever new metric data
 * is available during an active collection.
 *
 * @param[in] collector Handle to the collector for which to set the callback.
 * @param[in] data_callback Function pointer to the callback that will receive data records.
 *
 * @return UC_SUCCESS if the callback was set successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 * @return UC_ERROR_FUNCTION_NOT_SUPPORTED if the collector does not support this functionality.
 * @return UC_ERROR_COLLECTION_STARTED if collection is already running and callback cannot be changed.
 */
uc_result_t UCI_EXPORT uciSetDataCallback(uciCollectorHandle collector, void (*data_callback)(uciDataBundle* data_bundle));

/**
 * @brief Starts metric collection on all configured devices.
 *
 * This function begins collecting metrics based on the previously configured settings.
 * Collection will continue until uciStopCollection() is called.
 *
 * @param[in] collector Handle to the collector to start.
 *
 * @return UC_SUCCESS if collection started successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 * @return UC_ERROR_NO_CONFIGURATION if collection was not configured before starting.
 * @return UC_ERROR_COLLECTION_STARTED if collection is already running.
 */
uc_result_t UCI_EXPORT uciStartCollection(uciCollectorHandle collector);

/**
 * @brief Starts metric collection in a paused state.
 *
 * This function begins metric collection but immediately places it in a paused state.
 * This allows for collection setup and initialization without immediately starting
 * data flow. Collection can be resumed using uciResumeCollection().
 *
 * @param[in] collector Handle to the collector to start in paused state.
 *
 * @return UC_SUCCESS if collection started and paused successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 * @return UC_ERROR_NO_CONFIGURATION if collection was not configured before starting.
 * @return UC_ERROR_COLLECTION_STARTED if collection is already running.
 * @return UC_ERROR_FUNCTION_NOT_SUPPORTED if the collector does not support paused starts.
 */
uc_result_t UCI_EXPORT uciStartCollectionPaused(uciCollectorHandle collector);

/**
 * @brief Stops metric collection on all devices.
 *
 * This function stops all active metric collection and finalizes any pending data.
 * After stopping, the collection can be reconfigured and started again.
 *
 * @param[in] collector Handle to the collector to stop.
 *
 * @return UC_SUCCESS if collection stopped successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 * @return UC_ERROR_INVALID_COLLECTOR_HANDLE if the collector handle is invalid.
 * @return UC_ERROR_COLLECTION_NOT_STARTED if collection was not running.
 */
uc_result_t UCI_EXPORT uciStopCollection(uciCollectorHandle collector);

/**
 * @brief Temporarily pauses metric collection without losing configuration.
 *
 * This function suspends active metric collection while preserving all configuration.
 * Collection can be resumed by calling uciResumeCollection().
 *
 * @param[in] collector Handle to the collector to pause.
 *
 * @return UC_SUCCESS if collection paused successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 * @return UC_ERROR_COLLECTION_NOT_STARTED if collection was not running.
 * @return UC_ERROR_COLLECTION_ALREADY_PAUSED if collection is already paused.
 */
uc_result_t UCI_EXPORT uciPauseCollection(uciCollectorHandle collector);

/**
 * @brief Resumes previously paused metric collection.
 *
 * This function resumes metric collection that was previously paused,
 * using the same configuration that was active before pausing.
 *
 * @param[in] collector Handle to the collector to resume.
 *
 * @return UC_SUCCESS if collection resumed successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 * @return UC_ERROR_COLLECTION_NOT_PAUSED if collection is not currently paused.
 */
uc_result_t UCI_EXPORT uciResumeCollection(uciCollectorHandle collector);

/**
 * @brief Aggregates collected metric data into the specified output format.
 *
 * This function processes raw collected data from temporary storage,
 * performs any necessary calculations or transformations, and produces
 * a consolidated output in the requested format.
 *
 * @param[in] collector Handle to the collector whose data should be aggregated.
 *
 * @return UC_SUCCESS if data was successfully aggregated and saved.
 * @return UC_ERROR_INVALID_PARAMETER if collector, output_format, or output_path is NULL.
 * @return UC_ERROR_NO_DATA if no collection data is available to aggregate.
 */
uc_result_t UCI_EXPORT uciAggregateData(uciCollectorHandle collector);

/**
 * @brief Destroys the collector and frees all associated resources.
 *
 * This function performs cleanup of all resources associated with the collector,
 * including stopping any active collection, freeing memory, and releasing system
 * resources. After calling this function, the collector handle should not be used.
 *
 * @param[in] collector Handle to the collector to destroy.
 *
 * @return UC_SUCCESS if the collector was destroyed successfully.
 * @return UC_ERROR_INVALID_PARAMETER if collector is NULL.
 *
 * @note This function should be called when the collector is no longer needed
 *       to ensure proper cleanup of system resources. It's safe to call this
 *       function multiple times on the same handle.
 */
uc_result_t UCI_EXPORT uciDestroy(uciCollectorHandle collector);

#if defined(__cplusplus)
}
#endif

#endif // UCI_H_

