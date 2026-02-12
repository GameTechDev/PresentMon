#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_STATUS(X_) \
		X_(STATUS, SUCCESS, "Success", "", "Operation succeeded") \
		X_(STATUS, FAILURE, "Failure", "", "Operation failed") \
		X_(STATUS, BAD_ARGUMENT, "Bad Argument", "", "API function was called with invalid argument(s)") \
		X_(STATUS, BAD_HANDLE, "Bad Handle", "", "API function was called with an invalid handle") \
		X_(STATUS, SESSION_NOT_OPEN, "Session Not Open", "", "Operation requires an open session") \
		X_(STATUS, SERVICE_ERROR, "Service Error", "", "An error occurred within the service") \
		X_(STATUS, INVALID_ETL_FILE, "Invalid ETL File", "", "") \
		X_(STATUS, INVALID_PID, "Invalid PID", "", "PID does not exist or does not match a process being tracked") \
		X_(STATUS, ALREADY_TRACKING_PROCESS, "Already Tracking Process", "", "Tried to track a process already being tracked") \
		X_(STATUS, UNABLE_TO_CREATE_NSM, "Unable to Create NSM", "", "Service failed to create shrared memory for data transfer") \
		X_(STATUS, INVALID_ADAPTER_ID, "Invalid Adapter ID", "", "Graphics adapter ID provided does not match any ID in service") \
		X_(STATUS, OUT_OF_RANGE, "Out of Range", "", "Value falls outside of the acceptable range") \
		X_(STATUS, INSUFFICIENT_BUFFER, "Insufficient Buffer", "", "Buffer is not large enough to hold all output data") \
		X_(STATUS, PIPE_ERROR, "Pipe Error", "", "An error occurred in connecting to or communicating over named pipes") \
		X_(STATUS, MIDDLEWARE_MISSING_PATH, "Middleware Missing Path", "", "The path to the Middleware DLL was not found in the registry") \
		X_(STATUS, NONEXISTENT_FILE_PATH, "Nonexistent File Path", "", "The provided path does not point to a file or directory that exists") \
		X_(STATUS, MIDDLEWARE_INVALID_SIGNATURE, "Middleware Invalid Signature", "", "The DLL was not properly signed or was tampered with") \
		X_(STATUS, MIDDLEWARE_MISSING_ENDPOINT, "Middleware Missing Endpoint", "", "A required endpoint function was not found in the Middleware DLL") \
		X_(STATUS, MIDDLEWARE_VERSION_LOW, "Middleware Version Low", "", "Middleware DLL version was found to be too low for compatibility") \
		X_(STATUS, MIDDLEWARE_VERSION_HIGH, "Middleware Version High", "", "Middleware DLL version was found to be too high for compatibility") \
		X_(STATUS, MIDDLEWARE_SERVICE_MISMATCH, "Middleware Service Mismatch", "", "Middleware DLL build ID does not match that of the service") \
		X_(STATUS, QUERY_MALFORMED, "Query Malformed", "", "There are errors or inconsistencies in the elements of the query being registered") \
		X_(STATUS, MODE_MISMATCH, "Mode Mismatch", "", "Operation is not valid for the current service mode")
