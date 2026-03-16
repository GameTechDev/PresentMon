#ifndef _UCI_DATA_RECORDS_H_
#define _UCI_DATA_RECORDS_H_

#include <stdint.h>
#include <uci/uci-defines.h>
#include <uci/uci_export.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Handle to a metric record */
typedef struct rec_metric_continuous_t* uciMetricRecordHandle;

typedef struct _uciDataBundle {
    uint16_t numRecords;
    uciMetricRecordHandle* records;
} uciDataBundle;

/***************************
 * Record Access Functions *
 ***************************/

/**
 * @brief Retrieves the name of a metric record.
 *
 * @param record Handle to the metric record
 * @param name Pointer to store the retrieved name string
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT
uciMetricRecordGetMetricName(uciMetricRecordHandle record, char** name);

/**
 * @brief Retrieves the entity associated with a metric record.
 *
 * @param record Handle to the metric record
 * @param entity Pointer to store the retrieved entity string
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT uciMetricRecordGetEntity(uciMetricRecordHandle record,
                                                char** entity);

/**
 * @brief Retrieves the descriptor of a metric record.
 *
 * @param record Handle to the metric record
 * @param descriptor Pointer to store the retrieved descriptor string
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT
uciMetricRecordGetDescriptor(uciMetricRecordHandle record, char** descriptor);

/**
 * @brief Retrieves the unit of measurement for a metric record.
 *
 * @param record Handle to the metric record
 * @param unit Pointer to store the retrieved unit string
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT uciMetricRecordGetUnit(uciMetricRecordHandle record,
                                              char** unit);

/**
 * @brief Retrieves the sample value from a metric record.
 *
 * @param record Handle to the metric record
 * @param sample Pointer to store the retrieved sample value
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT uciMetricRecordGetSample(uciMetricRecordHandle record,
                                                double* sample);

/**
 * @brief Retrieves the timestamp from a metric record.
 *
 * @param record Handle to the metric record
 * @param timestamp Pointer to store the retrieved timestamp value
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT uciMetricRecordGetRecordTimestamp(
    uciMetricRecordHandle record, uint64_t* timestamp);

/**
 * @brief Retrieves the duration from a metric record.
 *
 * @param record Handle to the metric record
 * @param duration Pointer to store the retrieved duration value
 * @return uc_result_t Result code indicating success or failure
 */
uc_result_t UCI_EXPORT uciMetricRecordGetRecordDuration(
    uciMetricRecordHandle record, uint64_t* duration);

#if defined(__cplusplus)
}
#endif

#endif  // _UCI_DATA_RECORDS_H_
