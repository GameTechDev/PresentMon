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
 * @param[in] metricEvent Handle to the metric event whose ID should be retrieved.
 * @param[out] eventId Pointer to receive the event's unique identifier.
 *
 * @return UC_SUCCESS if the event ID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricEvent or eventId is NULL.
 */
uc_result_t UCI_EXPORT uciGetEventId(uciMetricEventHandle metricEvent, uint32_t* eventId);

/**
 * @brief Retrieves the name of the specified metric event.
 *
 * This function returns a human-readable name string that identifies the metric event.
 * The name provides descriptive information about what the event measures or represents.
 *
 * @param[in] metricEvent Handle to the metric event whose name should be retrieved.
 * @param[out] eventName Pointer to receive the event's name string.
 *
 * @return UC_SUCCESS if the event name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricEvent or eventName is NULL.
 *
 * @note The returned string is owned by the event and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricEventName(uciMetricEventHandle metricEvent, char** eventName);

/**
 * @brief Retrieves the description of the specified metric event.
 *
 * This function returns a detailed description string that explains what the metric
 * event measures.
 *
 * @param[in] metricEvent Handle to the metric event whose description should be retrieved.
 * @param[out] eventDescription Pointer to receive the event's description string.
 *
 * @return UC_SUCCESS if the event description was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricEvent or eventDescription is NULL.
 *
 * @note The returned string is owned by the event and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricEventDescription(uciMetricEventHandle metricEvent, char** eventDescription);

/**
 * @brief Retrieves the unique identifier of the specified metric.
 *
 * This function returns the metric ID that uniquely identifies the metric
 * within the system.
 *
 * @param[in] metric Handle to the metric whose ID should be retrieved.
 * @param[out] metricId Pointer to receive the metric's unique identifier.
 *
 * @return UC_SUCCESS if the metric ID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric or metricId is NULL.
 */
uc_result_t UCI_EXPORT uciGetMetricId(uciMetricHandle metric, uint32_t* metricId);

/***************************
 * Metric Access Functions *
 ***************************/

/**
 * @brief Retrieves the name of the specified metric.
 *
 * This function returns a human-readable name string that identifies the metric.
 *
 * @param[in] metric Handle to the metric whose name should be retrieved.
 * @param[out] metricName Pointer to receive the metric's name string.
 *
 * @return UC_SUCCESS if the metric name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric or metricName is NULL.
 *
 * @note The returned string is owned by the metric and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricName(uciMetricHandle metric, char** metricName);

/**
 * @brief Retrieves the description of the specified metric.
 *
 * This function returns a detailed description string that explains what the metric
 * measures.
 *
 * @param[in] metric Handle to the metric whose description should be retrieved.
 * @param[out] metricDescription Pointer to receive the metric's description string.
 *
 * @return UC_SUCCESS if the metric description was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric or metricDescription is NULL.
 *
 * @note The returned string is owned by the metric and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricDescription(uciMetricHandle metric, char** metricDescription);

/**
 * @brief Retrieves all events associated with the specified metric.
 *
 * This function returns an array of metric event handles that are associated with
 * the specified metric. These events represent the underlying hardware counters
 * or data sources that contribute to the metric calculation.
 *
 * @param[in] metric Handle to the metric whose events should be retrieved.
 * @param[out] eventCount Pointer to receive the number of events in the array.
 * @param[out] events Pointer to receive the array of metric event handles.
 *
 * @return UC_SUCCESS if the metric events were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metric, eventCount, or events is NULL.
 *
 * @note The returned array is owned by the metric and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricEvents(uciMetricHandle metric, uint32_t* eventCount, uciMetricEventHandle** events);

/*********************************
 * Metric Group Access Functions *
 *********************************/

/**
 * @brief Retrieves the unique identifier of the specified metric group.
 *
 * This function returns the group ID that uniquely identifies the metric group
 * within the system.
 *
 * @param[in] metricGroup Handle to the metric group whose ID should be retrieved.
 * @param[out] groupId Pointer to receive the group's unique identifier.
 *
 * @return UC_SUCCESS if the group ID was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricGroup or groupId is NULL.
 */
uc_result_t UCI_EXPORT uciGetMetricGroupId(uciMetricGroupHandle metricGroup, uint32_t* groupId);

/**
 * @brief Retrieves the name of the specified metric group.
 *
 * This function returns a human-readable name string that identifies the metric group.
 * The name provides descriptive information about the category or purpose of
 * the metrics contained within the group.
 *
 * @param[in] metricGroup Handle to the metric group whose name should be retrieved.
 * @param[out] metricGroupName Pointer to receive the group's name string.
 *
 * @return UC_SUCCESS if the group name was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricGroup or metricGroupName is NULL.
 *
 * @note The returned string is owned by the group and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricGroupName(uciMetricGroupHandle metricGroup, char** metricGroupName);

/**
 * @brief Retrieves the description of the specified metric group.
 *
 * This function returns a detailed description string that explains the purpose
 * of the metric group, and what types of metrics it contains.
 *
 * @param[in] metricGroup Handle to the metric group whose description should be retrieved.
 * @param[out] metricGroupDescription Pointer to receive the group's description string.
 *
 * @return UC_SUCCESS if the group description was retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricGroup or metricGroupDescription is NULL.
 *
 * @note The returned string is owned by the group and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricGroupDescription(uciMetricGroupHandle metricGroup, char** metricGroupDescription);

/**
 * @brief Retrieves all metrics contained within the specified metric group.
 *
 * This function returns an array of metric handles that belong to the specified
 * metric group.
 *
 * @param[in] metricGroup Handle to the metric group whose metrics should be retrieved.
 * @param[out] metricCount Pointer to receive the number of metrics in the group.
 * @param[out] metrics Pointer to receive the array of metric handles.
 *
 * @return UC_SUCCESS if the group metrics were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricGroup, metricCount, or metrics is NULL.
 *
 * @note The returned array is owned by the group and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricsInGroup(uciMetricGroupHandle metricGroup, uint32_t* metricCount, uciMetricHandle** metrics);

/**************************************
 * Metric Countainer Access Functions *
 **************************************/

/**
 * @brief Retrieves all metric groups within the specified metric container.
 *
 * This function returns an array of metric group handles that are contained
 * within the specified metric container.
 *
 * @param[in] metricContainer Handle to the metric container whose groups should be retrieved.
 * @param[out] groupCount Pointer to receive the number of groups in the container.
 * @param[out] groups Pointer to receive the array of metric group handles.
 *
 * @return UC_SUCCESS if the container groups were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricContainer, groupCount, or groups is NULL.
 *
 * @note The returned array is owned by the container and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricContainerGroups(uciMetricContainerHandle metricContainer, uint32_t* groupCount, uciMetricGroupHandle** groups);

/**
 * @brief Retrieves all individual metrics within the specified metric container.
 *
 * This function returns an array of metric handles for all individual metrics
 * contained within the specified metric container.
 *
 * @param[in] metricContainer Handle to the metric container whose metrics should be retrieved.
 * @param[out] metricCount Pointer to receive the number of metrics in the container.
 * @param[out] metrics Pointer to receive the array of metric handles.
 *
 * @return UC_SUCCESS if the container metrics were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricContainer, metricCount, or metrics is NULL.
 *
 * @note The returned array is owned by the container and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricContainerMetrics(uciMetricContainerHandle metricContainer, uint32_t* metricCount, uciMetricHandle** metrics);

/**
 * @brief Retrieves all events within the specified metric container.
 *
 * This function returns an array of metric event handles for all events
 * contained within the specified metric container.
 *
 * @param[in] metricContainer Handle to the metric container whose events should be retrieved.
 * @param[out] eventCount Pointer to receive the number of events in the container.
 * @param[out] events Pointer to receive the array of metric event handles.
 *
 * @return UC_SUCCESS if the container events were retrieved successfully.
 * @return UC_ERROR_INVALID_PARAMETER if metricContainer, eventCount, or events is NULL.
 *
 * @note The returned array is owned by the container and should not be modified or freed.
 */
uc_result_t UCI_EXPORT uciGetMetricContainerEvents(uciMetricContainerHandle metricContainer, uint32_t* eventCount, uciMetricEventHandle** events);

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
 * @param[in] metricContainer Handle to the metric container to be freed.
 *
 * @note Freeing a container also invalidates all handles obtained from it (groups, metrics, events).
 */
void UCI_EXPORT uciFreeMetricContainer(uciMetricContainerHandle metricContainer);

#if defined(__cplusplus)
}
#endif

#endif  // _UCI_METRICS_H_

