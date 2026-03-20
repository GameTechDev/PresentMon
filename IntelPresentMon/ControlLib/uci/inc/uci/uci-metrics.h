#ifndef _UCI_METRICS_H_
#define _UCI_METRICS_H_

#include <stdint.h>

#include <uci/uci-defines.h>
#include <uci/uci_export.h>

#if defined(__cplusplus)
extern "C" {
#endif

/***********************************
 * UCI Metric/Event Handles *
 ***********************************/

/** Handle to a specific metric event occurrence */
typedef struct uciMetricEvent* uciMetricEventHandle;

/** Handle to an individual metric definition */
typedef struct uciMetric* uciMetricHandle;

/** Handle to a group of related metrics */
typedef struct uciMetricGroup* uciMetricGroupHandle;

/** Handle to a container that manages multiple metric groups */
typedef struct uciMetricContainer* uciMetricContainerHandle;

/**************************
 * Event Access Functions *
 **************************/

/**
 * @brief Retrieves the unique identifier of the specified metric event.
 *
 * This function returns the event ID that uniquely identifies the metric event
 * within the system.
 *
 * @param[in] metric_event Handle to the metric event whose ID should be retrieved.
 * @param[out] event_id Pointer to receive the event's unique identifier.
 *
 * @return UC_SUCCESS if the event ID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_event or event_id is NULL.
 */
uc_result_t UCI_EXPORT uciGetEventId(uciMetricEventHandle metric_event, uint32_t* event_id);

/**
 * @brief Retrieves the name of the specified metric event.
 *
 * This function returns a human-readable name string that identifies the metric event.
 * The name provides descriptive information about what the event measures or represents.
 *
 * @param[in] metric_event Handle to the metric event whose name should be retrieved.
 * @param[out] event_name Pointer to receive the event's name string.
 *
 * @return UC_SUCCESS if the event name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_event or event_name is NULL.
 *
 * @note The returned string is owned by the event and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricEventName(uciMetricEventHandle metric_event, char** event_name);

/**
 * @brief Retrieves the description of the specified metric event.
 *
 * This function returns a detailed description string that explains what the metric
 * event measures.
 *
 * @param[in] metric_event Handle to the metric event whose description should be retrieved.
 * @param[out] event_description Pointer to receive the event's description string.
 *
 * @return UC_SUCCESS if the event description was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_event or event_description is NULL.
 *
 * @note The returned string is owned by the event and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricEventDescription(uciMetricEventHandle metric_event, char** event_description);

/**
 * @brief Retrieves the unique identifier of the specified metric.
 *
 * This function returns the metric ID that uniquely identifies the metric
 * within the system.
 *
 * @param[in] metric Handle to the metric whose ID should be retrieved.
 * @param[out] metric_id Pointer to receive the metric's unique identifier.
 *
 * @return UC_SUCCESS if the metric ID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric or metric_id is NULL.
 */
uc_result_t UCI_EXPORT uciGetMetricId(uciMetricHandle metric, uint32_t* metric_id);

/***************************
 * Metric Access Functions *
 ***************************/

/**
 * @brief Retrieves the name of the specified metric.
 *
 * This function returns a human-readable name string that identifies the metric.
 *
 * @param[in] metric Handle to the metric whose name should be retrieved.
 * @param[out] metric_name Pointer to receive the metric's name string.
 *
 * @return UC_SUCCESS if the metric name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric or metric_name is NULL.
 *
 * @note The returned string is owned by the metric and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricName(uciMetricHandle metric, char** metric_name);

/**
 * @brief Retrieves the description of the specified metric.
 *
 * This function returns a detailed description string that explains what the metric
 * measures.
 *
 * @param[in] metric Handle to the metric whose description should be retrieved.
 * @param[out] metric_description Pointer to receive the metric's description string.
 *
 * @return UC_SUCCESS if the metric description was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric or metric_description is NULL.
 *
 * @note The returned string is owned by the metric and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricDescription(uciMetricHandle metric, char** metric_description);

/**
 * @brief Retrieves all events associated with the specified metric.
 *
 * This function returns an array of metric event handles that are associated with
 * the specified metric. These events represent the underlying hardware counters
 * or data sources that contribute to the metric calculation.
 *
 * @param[in] metric Handle to the metric whose events should be retrieved.
 * @param[out] event_count Pointer to receive the number of events in the array.
 * @param[out] events Pointer to receive the array of metric event handles.
 *
 * @return UC_SUCCESS if the metric events were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric, event_count, or events is NULL.
 *
 * @note The returned array is owned by the metric and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricEvents(uciMetricHandle metric, uint32_t* event_count, uciMetricEventHandle** events);

/*********************************
 * Metric Group Access Functions *
 *********************************/

/**
 * @brief Retrieves the unique identifier of the specified metric group.
 *
 * This function returns the group ID that uniquely identifies the metric group
 * within the system.
 *
 * @param[in] metric_group Handle to the metric group whose ID should be retrieved.
 * @param[out] group_id Pointer to receive the group's unique identifier.
 *
 * @return UC_SUCCESS if the group ID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_group or group_id is NULL.
 */
uc_result_t UCI_EXPORT uciGetMetricGroupId(uciMetricGroupHandle metric_group, uint32_t* group_id);

/**
 * @brief Retrieves the name of the specified metric group.
 *
 * This function returns a human-readable name string that identifies the metric group.
 * The name provides descriptive information about the category or purpose of
 * the metrics contained within the group.
 *
 * @param[in] metric_group Handle to the metric group whose name should be retrieved.
 * @param[out] metric_group_name Pointer to receive the group's name string.
 *
 * @return UC_SUCCESS if the group name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_group or metric_group_name is NULL.
 *
 * @note The returned string is owned by the group and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricGroupName(uciMetricGroupHandle metric_group, char** metric_group_name);

/**
 * @brief Retrieves the description of the specified metric group.
 *
 * This function returns a detailed description string that explains the purpose
 * of the metric group, and what types of metrics it contains.
 *
 * @param[in] metric_group Handle to the metric group whose description should be retrieved.
 * @param[out] metric_group_description Pointer to receive the group's description string.
 *
 * @return UC_SUCCESS if the group description was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_group or metric_group_description is NULL.
 *
 * @note The returned string is owned by the group and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricGroupDescription(uciMetricGroupHandle metric_group, char** metric_group_description);

/**
 * @brief Retrieves all metrics contained within the specified metric group.
 *
 * This function returns an array of metric handles that belong to the specified
 * metric group.
 *
 * @param[in] metric_group Handle to the metric group whose metrics should be retrieved.
 * @param[out] metric_count Pointer to receive the number of metrics in the group.
 * @param[out] metrics Pointer to receive the array of metric handles.
 *
 * @return UC_SUCCESS if the group metrics were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_group, metric_count, or metrics is NULL.
 *
 * @note The returned array is owned by the group and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricsInGroup(uciMetricGroupHandle metric_group, uint32_t* metric_count, uciMetricHandle** metrics);

/**************************************
 * Metric Container Access Functions *
 **************************************/

/**
 * @brief Retrieves all metric groups within the specified metric container.
 *
 * This function returns an array of metric group handles that are contained
 * within the specified metric container.
 *
 * @param[in] metric_container Handle to the metric container whose groups should be retrieved.
 * @param[out] group_count Pointer to receive the number of groups in the container.
 * @param[out] groups Pointer to receive the array of metric group handles.
 *
 * @return UC_SUCCESS if the container groups were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_container, group_count, or groups is NULL.
 *
 * @note The returned array is owned by the container and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricContainerGroups(uciMetricContainerHandle metric_container, uint32_t* group_count, uciMetricGroupHandle** groups);

/**
 * @brief Retrieves all individual metrics within the specified metric container.
 *
 * This function returns an array of metric handles for all individual metrics
 * contained within the specified metric container.
 *
 * @param[in] metric_container Handle to the metric container whose metrics should be retrieved.
 * @param[out] metric_count Pointer to receive the number of metrics in the container.
 * @param[out] metrics Pointer to receive the array of metric handles.
 *
 * @return UC_SUCCESS if the container metrics were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_container, metric_count, or metrics is NULL.
 *
 * @note The returned array is owned by the container and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricContainerMetrics(uciMetricContainerHandle metric_container, uint32_t* metric_count, uciMetricHandle** metrics);

/**
 * @brief Retrieves all events within the specified metric container.
 *
 * This function returns an array of metric event handles for all events
 * contained within the specified metric container.
 *
 * @param[in] metric_container Handle to the metric container whose events should be retrieved.
 * @param[out] event_count Pointer to receive the number of events in the container.
 * @param[out] events Pointer to receive the array of metric event handles.
 *
 * @return UC_SUCCESS if the container events were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric_container, event_count, or events is NULL.
 *
 * @note The returned array is owned by the container and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricContainerEvents(uciMetricContainerHandle metric_container, uint32_t* event_count, uciMetricEventHandle** events);

/********************************
 * Metric/Device Free Functions *
 ********************************/

/**
 * @brief Frees resources associated with a metric container handle.
 *
 * This function releases all memory and resources associated with the specified
 * metric container handle, including all contained metric groups, metrics, and events.
 * After calling this function, the metric container handle and any handles obtained
 * from it should not be used. This function should be called when the metric container
 * is no longer needed to ensure proper cleanup of system resources.
 *
 * @param[in] metric_container Handle to the metric container to be freed.
 *
 * @note Freeing a container also invalidates all handles obtained from it (groups, metrics, events).
 */
void UCI_EXPORT uciFreeMetricContainer(uciMetricContainerHandle metric_container);

#if defined(__cplusplus)
}
#endif

#endif  // _UCI_METRICS_H_

