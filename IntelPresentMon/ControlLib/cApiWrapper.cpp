//===========================================================================
//Copyright (C) 2025 Intel Corporation
//
// 
//
//SPDX-License-Identifier: MIT
//--------------------------------------------------------------------------

/**
 *
 * @file ctl_api.cpp
 * @version v1-r1
 *
 */

// Note: UWP applications should have defined WINDOWS_UWP in their compiler settings
// Also at this point, it's easier by not enabling pre-compiled option to compile this file
// Not all functionalities are tested for a UWP application

#include <windows.h>
#include <strsafe.h>
#include <vector>

//#define CTL_APIEXPORT

#include "igcl_api.h"

/////////////////////////////////////////////////////////////////////////////////
//
// Implementation of wrapper functions
//
static HINSTANCE hinstLib = NULL;
static ctl_runtime_path_args_t* pRuntimeArgs = NULL;

HINSTANCE GetLoaderHandle(void)
{
    return hinstLib;
}

/**
 * @brief Function to get DLL name based on app version
 *
 */

#if defined(_WIN64)
    #define CTL_DLL_NAME L"ControlLib"
#else
    #define CTL_DLL_NAME L"ControlLib32"
#endif
#define CTL_DLL_PATH_LEN 512

ctl_result_t GetControlAPIDLLPath(ctl_init_args_t* pInitArgs, wchar_t* pwcDLLPath)
{
    if ((NULL == pRuntimeArgs) || (NULL == pRuntimeArgs->pRuntimePath))
    {
        // Load the requested DLL based on major version in init args
        uint16_t majorVersion = CTL_MAJOR_VERSION(pInitArgs->AppVersion);

        // If caller's major version is higher than the DLL's, then simply not support the caller!
        // This is not supposed to happen as wrapper is part of the app itself which includes igcl_api.h with right major version
        if (majorVersion > CTL_IMPL_MAJOR_VERSION)
            return CTL_RESULT_ERROR_UNSUPPORTED_VERSION;

#if (CTL_IMPL_MAJOR_VERSION > 1)
        if (majorVersion > 1)
            StringCbPrintfW(pwcDLLPath,CTL_DLL_PATH_LEN,L"%s%d.dll", CTL_DLL_NAME, majorVersion);
        else // just control_api.dll
            StringCbPrintfW(pwcDLLPath,CTL_DLL_PATH_LEN,L"%s.dll", CTL_DLL_NAME);
#else
        StringCbPrintfW(pwcDLLPath,CTL_DLL_PATH_LEN,L"%s.dll", CTL_DLL_NAME);
#endif

    }
    else if (pRuntimeArgs->pRuntimePath)
    {
        // caller specified a specific RT, use it instead
        wcsncpy_s(pwcDLLPath, CTL_DLL_PATH_LEN, pRuntimeArgs->pRuntimePath, CTL_DLL_PATH_LEN - 1);
    }
    return CTL_RESULT_SUCCESS;
}



/**
* @brief Control Api Init
* 
* @details
*     - Control Api Init
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pInitDesc`
*         + `nullptr == phAPIHandle`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlInit(
    ctl_init_args_t* pInitDesc,                     ///< [in][out] App's control API version
    ctl_api_handle_t* phAPIHandle                   ///< [in][out][release] Control API handle
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    
    // special code - only for ctlInit()
    if (NULL == hinstLib)
    {
        std::vector<wchar_t> strDLLPath;
        try
        {
            strDLLPath.resize(CTL_DLL_PATH_LEN);
        }
        catch (std::bad_alloc&)
        {
            return CTL_RESULT_ERROR_OUT_OF_DEVICE_MEMORY;
        }

        result = GetControlAPIDLLPath(pInitDesc, strDLLPath.data());
        if (result == CTL_RESULT_SUCCESS)
        {
#ifdef WINDOWS_UWP
            hinstLib = LoadPackagedLibrary(strDLLPath.data(), 0);
#else
            DWORD dwFlags = LOAD_LIBRARY_SEARCH_SYSTEM32;
#ifdef _DEBUG
            dwFlags = dwFlags | LOAD_LIBRARY_SEARCH_APPLICATION_DIR;
#endif
            hinstLib = LoadLibraryExW(strDLLPath.data(), NULL, dwFlags);
#endif
            if (NULL == hinstLib)
            {
                result = CTL_RESULT_ERROR_LOAD;
            }
            else if (pRuntimeArgs)
            {
                ctlSetRuntimePath(pRuntimeArgs);
            }
        }
    }

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnInit_t pfnInit = (ctl_pfnInit_t)GetProcAddress(hinstLibPtr, "ctlInit");
        if (pfnInit)
        {
            result = pfnInit(pInitDesc, phAPIHandle);
        }
    }

    return result;
}


/**
* @brief Control Api Destroy
* 
* @details
*     - Control Api Close
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hAPIHandle`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlClose(
    ctl_api_handle_t hAPIHandle                     ///< [in][release] Control API implementation handle obtained during init
                                                    ///< call
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnClose_t pfnClose = (ctl_pfnClose_t)GetProcAddress(hinstLibPtr, "ctlClose");
        if (pfnClose)
        {
            result = pfnClose(hAPIHandle);
        }
    }

    // special code - only for ctlClose()
    // might get CTL_RESULT_SUCCESS_STILL_OPEN_BY_ANOTHER_CALLER
    // if its open by another caller do not free the instance handle 
    if( result == CTL_RESULT_SUCCESS)
    {
        if (NULL != hinstLib)
        {
            FreeLibrary(hinstLib);
            hinstLib = NULL;
        }        
    }
    // set runtime args back to NULL
    // no need to free this as it's allocated by caller   
    pRuntimeArgs = NULL;
    return result;
}


/**
* @brief Runtime path
* 
* @details
*     - Control Api set runtime path. Optional call from a loader which allows
*       the loaded runtime to enumerate only the adapters which the specified
*       runtime is responsible for. This is done usually by a loader or by
*       callers who know how to get the specific runtime of interest. This
*       call right now is reserved for use by Intel components.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlSetRuntimePath(
    ctl_runtime_path_args_t* pArgs                  ///< [in] Runtime path
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetRuntimePath_t pfnSetRuntimePath = (ctl_pfnSetRuntimePath_t)GetProcAddress(hinstLibPtr, "ctlSetRuntimePath");
        if (pfnSetRuntimePath)
        {
            result = pfnSetRuntimePath(pArgs);
        }
    }

    // special code - only for ctlSetRuntimePath()
    // might get CTL_RESULT_SUCCESS_STILL_OPEN_BY_ANOTHER_CALLER
    // if its open by another caller do not free the instance handle 
    else if (pArgs->pRuntimePath)
    {
        // this is a case where the caller app is interested in loading a RT directly
    // IMPORTANT NOTE: Free pArgs and pArgs->pRuntimePath only after ctlInit() call
        pRuntimeArgs = pArgs;
        result = CTL_RESULT_SUCCESS;
    }
    return result;
}


/**
* @brief Wait for a property change. Note that this is a blocking call
* 
* @details
*     - Wait for a property change in display, 3d, media etc.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlWaitForPropertyChange(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    ctl_wait_property_change_args_t* pArgs          ///< [in] Argument containing information about which property changes to
                                                    ///< listen for
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnWaitForPropertyChange_t pfnWaitForPropertyChange = (ctl_pfnWaitForPropertyChange_t)GetProcAddress(hinstLibPtr, "ctlWaitForPropertyChange");
        if (pfnWaitForPropertyChange)
        {
            result = pfnWaitForPropertyChange(hDeviceAdapter, pArgs);
        }
    }

    return result;
}


/**
* @brief Reserved function
* 
* @details
*     - Reserved function
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlReservedCall(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    ctl_reserved_args_t* pArgs                      ///< [in] Argument containing information
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnReservedCall_t pfnReservedCall = (ctl_pfnReservedCall_t)GetProcAddress(hinstLibPtr, "ctlReservedCall");
        if (pfnReservedCall)
        {
            result = pfnReservedCall(hDeviceAdapter, pArgs);
        }
    }

    return result;
}


/**
* @brief Get 3D capabilities
* 
* @details
*     - The application gets 3D properties
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pFeatureCaps`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSupported3DCapabilities(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_3d_feature_caps_t* pFeatureCaps             ///< [in,out][release] 3D properties
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSupported3DCapabilities_t pfnGetSupported3DCapabilities = (ctl_pfnGetSupported3DCapabilities_t)GetProcAddress(hinstLibPtr, "ctlGetSupported3DCapabilities");
        if (pfnGetSupported3DCapabilities)
        {
            result = pfnGetSupported3DCapabilities(hDAhandle, pFeatureCaps);
        }
    }

    return result;
}


/**
* @brief Get/Set 3D feature
* 
* @details
*     - 3D feature details
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pFeature`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSet3DFeature(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_3d_feature_getset_t* pFeature               ///< [in][release] 3D feature get/set parameter
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSet3DFeature_t pfnGetSet3DFeature = (ctl_pfnGetSet3DFeature_t)GetProcAddress(hinstLibPtr, "ctlGetSet3DFeature");
        if (pfnGetSet3DFeature)
        {
            result = pfnGetSet3DFeature(hDAhandle, pFeature);
        }
    }

    return result;
}


/**
* @brief Check Driver version
* 
* @details
*     - The application checks driver version
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlCheckDriverVersion(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    ctl_version_info_t version_info                 ///< [in][release] Driver version info
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnCheckDriverVersion_t pfnCheckDriverVersion = (ctl_pfnCheckDriverVersion_t)GetProcAddress(hinstLibPtr, "ctlCheckDriverVersion");
        if (pfnCheckDriverVersion)
        {
            result = pfnCheckDriverVersion(hDeviceAdapter, version_info);
        }
    }

    return result;
}


/**
* @brief Enumerate devices
* 
* @details
*     - The application enumerates all device adapters in the system
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hAPIHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlEnumerateDevices(
    ctl_api_handle_t hAPIHandle,                    ///< [in][release] Applications should pass the Control API handle returned
                                                    ///< by the CtlInit function 
    uint32_t* pCount,                               ///< [in,out][release] pointer to the number of device instances. If count
                                                    ///< is zero, then the api will update the value with the total
                                                    ///< number of drivers available. If count is non-zero, then the api will
                                                    ///< only retrieve the number of drivers.
                                                    ///< If count is larger than the number of drivers available, then the api
                                                    ///< will update the value with the correct number of drivers available.
    ctl_device_adapter_handle_t* phDevices          ///< [in,out][optional][release][range(0, *pCount)] array of driver
                                                    ///< instance handles
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumerateDevices_t pfnEnumerateDevices = (ctl_pfnEnumerateDevices_t)GetProcAddress(hinstLibPtr, "ctlEnumerateDevices");
        if (pfnEnumerateDevices)
        {
            result = pfnEnumerateDevices(hAPIHandle, pCount, phDevices);
        }
    }

    return result;
}


/**
* @brief Enumerate display outputs
* 
* @details
*     - Enumerates display output capabilities
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlEnumerateDisplayOutputs(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    uint32_t* pCount,                               ///< [in,out][release] pointer to the number of display output instances.
                                                    ///< If count is zero, then the api will update the value with the total
                                                    ///< number of outputs available. If count is non-zero, then the api will
                                                    ///< only retrieve the number of outputs.
                                                    ///< If count is larger than the number of drivers available, then the api
                                                    ///< will update the value with the correct number of drivers available.
    ctl_display_output_handle_t* phDisplayOutputs   ///< [in,out][optional][release][range(0, *pCount)] array of display output
                                                    ///< instance handles
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumerateDisplayOutputs_t pfnEnumerateDisplayOutputs = (ctl_pfnEnumerateDisplayOutputs_t)GetProcAddress(hinstLibPtr, "ctlEnumerateDisplayOutputs");
        if (pfnEnumerateDisplayOutputs)
        {
            result = pfnEnumerateDisplayOutputs(hDeviceAdapter, pCount, phDisplayOutputs);
        }
    }

    return result;
}


/**
* @brief Enumerate I2C Pin Pairs
* 
* @details
*     - Returns available list of I2C Pin-Pairs on a requested adapter
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "The incoming pointer pCount is null"
*     - ::CTL_RESULT_ERROR_INVALID_SIZE - "The supplied Count is not equal to actual number of i2c pin-pair instances"
*/
ctl_result_t CTL_APICALL
ctlEnumerateI2CPinPairs(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to device adapter
    uint32_t* pCount,                               ///< [in,out][release] pointer to the number of i2c pin-pair instances. If
                                                    ///< count is zero, then the api will update the value with the total
                                                    ///< number of i2c pin-pair instances available. If count is non-zero and
                                                    ///< matches the avaialble number of pin-pairs, then the api will only
                                                    ///< return the avaialble number of i2c pin-pair instances in phI2cPinPairs.
    ctl_i2c_pin_pair_handle_t* phI2cPinPairs        ///< [out][optional][release][range(0, *pCount)] array of i2c pin pair
                                                    ///< instance handles. Need to be allocated by Caller when supplying the
                                                    ///< *pCount > 0. 
                                                    ///< If Count is not equal to actual number of i2c pin-pair instances, it
                                                    ///< will return CTL_RESULT_ERROR_INVALID_SIZE.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumerateI2CPinPairs_t pfnEnumerateI2CPinPairs = (ctl_pfnEnumerateI2CPinPairs_t)GetProcAddress(hinstLibPtr, "ctlEnumerateI2CPinPairs");
        if (pfnEnumerateI2CPinPairs)
        {
            result = pfnEnumerateI2CPinPairs(hDeviceAdapter, pCount, phI2cPinPairs);
        }
    }

    return result;
}


/**
* @brief Get Device Properties
* 
* @details
*     - The application gets device properties
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetDeviceProperties(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to control device adapter
    ctl_device_adapter_properties_t* pProperties    ///< [in,out][release] Query result for device properties
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetDeviceProperties_t pfnGetDeviceProperties = (ctl_pfnGetDeviceProperties_t)GetProcAddress(hinstLibPtr, "ctlGetDeviceProperties");
        if (pfnGetDeviceProperties)
        {
            result = pfnGetDeviceProperties(hDAhandle, pProperties);
        }
    }

    return result;
}


/**
* @brief Get Display  Properties
* 
* @details
*     - The application gets display  properties
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetDisplayProperties(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_display_properties_t* pProperties           ///< [in,out][release] Query result for display  properties
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetDisplayProperties_t pfnGetDisplayProperties = (ctl_pfnGetDisplayProperties_t)GetProcAddress(hinstLibPtr, "ctlGetDisplayProperties");
        if (pfnGetDisplayProperties)
        {
            result = pfnGetDisplayProperties(hDisplayOutput, pProperties);
        }
    }

    return result;
}


/**
* @brief Get Adapter Display encoder  Properties
* 
* @details
*     - The application gets the graphic adapters display encoder properties
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetAdaperDisplayEncoderProperties(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_adapter_display_encoder_properties_t* pProperties   ///< [in,out][release] Query result for adapter display encoder properties
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetAdaperDisplayEncoderProperties_t pfnGetAdaperDisplayEncoderProperties = (ctl_pfnGetAdaperDisplayEncoderProperties_t)GetProcAddress(hinstLibPtr, "ctlGetAdaperDisplayEncoderProperties");
        if (pfnGetAdaperDisplayEncoderProperties)
        {
            result = pfnGetAdaperDisplayEncoderProperties(hDisplayOutput, pProperties);
        }
    }

    return result;
}


/**
* @brief Get Level0 Device handle
* 
* @details
*     - The application gets OneAPI Level0 Device handles
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pZeDevice`
*         + `nullptr == hInstance`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetZeDevice(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    void* pZeDevice,                                ///< [out][release] ze_device handle
    void** hInstance                                ///< [out][release] Module instance which caller can use to get export
                                                    ///< functions directly
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetZeDevice_t pfnGetZeDevice = (ctl_pfnGetZeDevice_t)GetProcAddress(hinstLibPtr, "ctlGetZeDevice");
        if (pfnGetZeDevice)
        {
            result = pfnGetZeDevice(hDAhandle, pZeDevice, hInstance);
        }
    }

    return result;
}


/**
* @brief Get Sharpness capability
* 
* @details
*     - Returns sharpness capability
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSharpnessCaps`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSharpnessCaps(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_sharpness_caps_t* pSharpnessCaps            ///< [in,out][release] Query result for sharpness capability
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSharpnessCaps_t pfnGetSharpnessCaps = (ctl_pfnGetSharpnessCaps_t)GetProcAddress(hinstLibPtr, "ctlGetSharpnessCaps");
        if (pfnGetSharpnessCaps)
        {
            result = pfnGetSharpnessCaps(hDisplayOutput, pSharpnessCaps);
        }
    }

    return result;
}


/**
* @brief Get Sharpness setting
* 
* @details
*     - Returns current sharpness settings
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSharpnessSettings`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetCurrentSharpness(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_sharpness_settings_t* pSharpnessSettings    ///< [in,out][release] Query result for sharpness current settings
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetCurrentSharpness_t pfnGetCurrentSharpness = (ctl_pfnGetCurrentSharpness_t)GetProcAddress(hinstLibPtr, "ctlGetCurrentSharpness");
        if (pfnGetCurrentSharpness)
        {
            result = pfnGetCurrentSharpness(hDisplayOutput, pSharpnessSettings);
        }
    }

    return result;
}


/**
* @brief Set Sharpness setting
* 
* @details
*     - Set current sharpness settings
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSharpnessSettings`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlSetCurrentSharpness(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_sharpness_settings_t* pSharpnessSettings    ///< [in][release] Set sharpness current settings
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetCurrentSharpness_t pfnSetCurrentSharpness = (ctl_pfnSetCurrentSharpness_t)GetProcAddress(hinstLibPtr, "ctlSetCurrentSharpness");
        if (pfnSetCurrentSharpness)
        {
            result = pfnSetCurrentSharpness(hDisplayOutput, pSharpnessSettings);
        }
    }

    return result;
}


/**
* @brief I2C Access
* 
* @details
*     - Interface to access I2C using display handle as identifier.  I2C
*       driver override flags are supported only for HDMI displays.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pI2cAccessArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_SIZE - "Invalid I2C data size"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*/
ctl_result_t CTL_APICALL
ctlI2CAccess(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_i2c_access_args_t* pI2cAccessArgs           ///< [in,out] I2c access arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnI2CAccess_t pfnI2CAccess = (ctl_pfnI2CAccess_t)GetProcAddress(hinstLibPtr, "ctlI2CAccess");
        if (pfnI2CAccess)
        {
            result = pfnI2CAccess(hDisplayOutput, pI2cAccessArgs);
        }
    }

    return result;
}


/**
* @brief I2C Access On Pin Pair
* 
* @details
*     - Interface to access I2C using pin-pair handle as identifier.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hI2cPinPair`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pI2cAccessArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_SIZE - "Invalid I2C data size"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid Args passed"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_HANDLE - "Invalid or Null handle passed"
*     - ::CTL_RESULT_ERROR_EXTERNAL_DISPLAY_ATTACHED - "Write to Address not allowed when Display is connected"
*/
ctl_result_t CTL_APICALL
ctlI2CAccessOnPinPair(
    ctl_i2c_pin_pair_handle_t hI2cPinPair,          ///< [in] Handle to I2C pin pair.
    ctl_i2c_access_pinpair_args_t* pI2cAccessArgs   ///< [in,out] I2c access arguments.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnI2CAccessOnPinPair_t pfnI2CAccessOnPinPair = (ctl_pfnI2CAccessOnPinPair_t)GetProcAddress(hinstLibPtr, "ctlI2CAccessOnPinPair");
        if (pfnI2CAccessOnPinPair)
        {
            result = pfnI2CAccessOnPinPair(hI2cPinPair, pI2cAccessArgs);
        }
    }

    return result;
}


/**
* @brief Aux Access
* 
* @details
*     - The application does Aux access, PSR needs to be disabled for AUX
*       call.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pAuxAccessArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_SIZE - "Invalid AUX data size"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_INVALID_AUX_ACCESS_FLAG - "Invalid flag for AUX access"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*/
ctl_result_t CTL_APICALL
ctlAUXAccess(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_aux_access_args_t* pAuxAccessArgs           ///< [in,out] Aux access arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnAUXAccess_t pfnAUXAccess = (ctl_pfnAUXAccess_t)GetProcAddress(hinstLibPtr, "ctlAUXAccess");
        if (pfnAUXAccess)
        {
            result = pfnAUXAccess(hDisplayOutput, pAuxAccessArgs);
        }
    }

    return result;
}


/**
* @brief Get Power optimization features
* 
* @details
*     - Returns power optimization capabilities
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pPowerOptimizationCaps`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetPowerOptimizationCaps(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_power_optimization_caps_t* pPowerOptimizationCaps   ///< [in,out][release] Query result for power optimization features
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetPowerOptimizationCaps_t pfnGetPowerOptimizationCaps = (ctl_pfnGetPowerOptimizationCaps_t)GetProcAddress(hinstLibPtr, "ctlGetPowerOptimizationCaps");
        if (pfnGetPowerOptimizationCaps)
        {
            result = pfnGetPowerOptimizationCaps(hDisplayOutput, pPowerOptimizationCaps);
        }
    }

    return result;
}


/**
* @brief Get Power optimization setting
* 
* @details
*     - Returns power optimization setting for a specific feature
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pPowerOptimizationSettings`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_POWERFEATURE_OPTIMIZATION_FLAG - "Unsupported PowerOptimizationFeature"
*     - ::CTL_RESULT_ERROR_INVALID_POWERSOURCE_TYPE_FOR_DPST - "DPST is supported only in DC Mode"
*/
ctl_result_t CTL_APICALL
ctlGetPowerOptimizationSetting(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_power_optimization_settings_t* pPowerOptimizationSettings   ///< [in,out][release] Power optimization data to be fetched
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetPowerOptimizationSetting_t pfnGetPowerOptimizationSetting = (ctl_pfnGetPowerOptimizationSetting_t)GetProcAddress(hinstLibPtr, "ctlGetPowerOptimizationSetting");
        if (pfnGetPowerOptimizationSetting)
        {
            result = pfnGetPowerOptimizationSetting(hDisplayOutput, pPowerOptimizationSettings);
        }
    }

    return result;
}


/**
* @brief Set Power optimization setting
* 
* @details
*     - Set power optimization setting for a specific feature
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pPowerOptimizationSettings`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_POWERFEATURE_OPTIMIZATION_FLAG - "Unsupported PowerOptimizationFeature"
*     - ::CTL_RESULT_ERROR_INVALID_POWERSOURCE_TYPE_FOR_DPST - "DPST is supported only in DC Mode"
*     - ::CTL_RESULT_ERROR_SET_FBC_FEATURE_NOT_SUPPORTED - "Set FBC Feature not supported"
*/
ctl_result_t CTL_APICALL
ctlSetPowerOptimizationSetting(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_power_optimization_settings_t* pPowerOptimizationSettings   ///< [in][release] Power optimization data to be applied
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetPowerOptimizationSetting_t pfnSetPowerOptimizationSetting = (ctl_pfnSetPowerOptimizationSetting_t)GetProcAddress(hinstLibPtr, "ctlSetPowerOptimizationSetting");
        if (pfnSetPowerOptimizationSetting)
        {
            result = pfnSetPowerOptimizationSetting(hDisplayOutput, pPowerOptimizationSettings);
        }
    }

    return result;
}


/**
* @brief Set Brightness on companion display
* 
* @details
*     - Set Brightness for a target display. Currently support is only for
*       companion display.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSetBrightnessSetting`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid Brightness data passed as argument"
*     - ::CTL_RESULT_ERROR_DISPLAY_NOT_ACTIVE - "Display not active"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Currently Brightness API is supported only on companion display"
*/
ctl_result_t CTL_APICALL
ctlSetBrightnessSetting(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_set_brightness_t* pSetBrightnessSetting     ///< [in][release] Brightness settings to be applied
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetBrightnessSetting_t pfnSetBrightnessSetting = (ctl_pfnSetBrightnessSetting_t)GetProcAddress(hinstLibPtr, "ctlSetBrightnessSetting");
        if (pfnSetBrightnessSetting)
        {
            result = pfnSetBrightnessSetting(hDisplayOutput, pSetBrightnessSetting);
        }
    }

    return result;
}


/**
* @brief Get Brightness setting
* 
* @details
*     - Get Brightness for a target display. Currently support is only for
*       companion display.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pGetBrightnessSetting`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_DISPLAY_NOT_ACTIVE - "Display not active"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Currently Brightness API is supported only on companion display"
*/
ctl_result_t CTL_APICALL
ctlGetBrightnessSetting(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_get_brightness_t* pGetBrightnessSetting     ///< [out][release] Brightness settings data to be fetched
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetBrightnessSetting_t pfnGetBrightnessSetting = (ctl_pfnGetBrightnessSetting_t)GetProcAddress(hinstLibPtr, "ctlGetBrightnessSetting");
        if (pfnGetBrightnessSetting)
        {
            result = pfnGetBrightnessSetting(hDisplayOutput, pGetBrightnessSetting);
        }
    }

    return result;
}


/**
* @brief Pixel transformation get pipe configuration
* 
* @details
*     - The application does pixel transformation get pipe configuration
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pPixTxGetConfigArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_PIXTX_GET_CONFIG_QUERY_TYPE - "Invalid query type"
*     - ::CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_ID - "Invalid block id"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PIXTX_BLOCK_CONFIG_MEMORY - "Insufficient memery allocated for BlockConfigs"
*     - ::CTL_RESULT_ERROR_3DLUT_INVALID_PIPE - "Invalid pipe for 3dlut"
*     - ::CTL_RESULT_ERROR_3DLUT_INVALID_DATA - "Invalid 3dlut data"
*     - ::CTL_RESULT_ERROR_3DLUT_NOT_SUPPORTED_IN_HDR - "3dlut not supported in HDR"
*     - ::CTL_RESULT_ERROR_3DLUT_INVALID_OPERATION - "Invalid 3dlut operation"
*     - ::CTL_RESULT_ERROR_3DLUT_UNSUCCESSFUL - "3dlut call unsuccessful"
*/
ctl_result_t CTL_APICALL
ctlPixelTransformationGetConfig(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_pixtx_pipe_get_config_t* pPixTxGetConfigArgs///< [in,out] Pixel transformation get pipe configiguration arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPixelTransformationGetConfig_t pfnPixelTransformationGetConfig = (ctl_pfnPixelTransformationGetConfig_t)GetProcAddress(hinstLibPtr, "ctlPixelTransformationGetConfig");
        if (pfnPixelTransformationGetConfig)
        {
            result = pfnPixelTransformationGetConfig(hDisplayOutput, pPixTxGetConfigArgs);
        }
    }

    return result;
}


/**
* @brief Pixel transformation set pipe configuration
* 
* @details
*     - The application does pixel transformation set pipe configuration
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pPixTxSetConfigArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_PIXTX_SET_CONFIG_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_SET_CONFIG_NUMBER_OF_SAMPLES - "Invalid number of samples"
*     - ::CTL_RESULT_ERROR_INVALID_PIXTX_BLOCK_ID - "Invalid block id"
*     - ::CTL_RESULT_ERROR_PERSISTANCE_NOT_SUPPORTED - "Persistance not supported"
*     - ::CTL_RESULT_ERROR_3DLUT_INVALID_PIPE - "Invalid pipe for 3dlut"
*     - ::CTL_RESULT_ERROR_3DLUT_INVALID_DATA - "Invalid 3dlut data"
*     - ::CTL_RESULT_ERROR_3DLUT_NOT_SUPPORTED_IN_HDR - "3dlut not supported in HDR"
*     - ::CTL_RESULT_ERROR_3DLUT_INVALID_OPERATION - "Invalid 3dlut operation"
*     - ::CTL_RESULT_ERROR_3DLUT_UNSUCCESSFUL - "3dlut call unsuccessful"
*/
ctl_result_t CTL_APICALL
ctlPixelTransformationSetConfig(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_pixtx_pipe_set_config_t* pPixTxSetConfigArgs///< [in,out] Pixel transformation set pipe configiguration arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPixelTransformationSetConfig_t pfnPixelTransformationSetConfig = (ctl_pfnPixelTransformationSetConfig_t)GetProcAddress(hinstLibPtr, "ctlPixelTransformationSetConfig");
        if (pfnPixelTransformationSetConfig)
        {
            result = pfnPixelTransformationSetConfig(hDisplayOutput, pPixTxSetConfigArgs);
        }
    }

    return result;
}


/**
* @brief Panel Descriptor Access
* 
* @details
*     - The application does EDID or Display ID access
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pPanelDescriptorAccessArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*/
ctl_result_t CTL_APICALL
ctlPanelDescriptorAccess(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_panel_descriptor_access_args_t* pPanelDescriptorAccessArgs  ///< [in,out] Panel descriptor access arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPanelDescriptorAccess_t pfnPanelDescriptorAccess = (ctl_pfnPanelDescriptorAccess_t)GetProcAddress(hinstLibPtr, "ctlPanelDescriptorAccess");
        if (pfnPanelDescriptorAccess)
        {
            result = pfnPanelDescriptorAccess(hDisplayOutput, pPanelDescriptorAccessArgs);
        }
    }

    return result;
}


/**
* @brief Get Supported Retro Scaling Types
* 
* @details
*     - Returns supported retro scaling capabilities
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pRetroScalingCaps`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSupportedRetroScalingCapability(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to adapter
    ctl_retro_scaling_caps_t* pRetroScalingCaps     ///< [in,out][release] Query result for supported retro scaling types
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSupportedRetroScalingCapability_t pfnGetSupportedRetroScalingCapability = (ctl_pfnGetSupportedRetroScalingCapability_t)GetProcAddress(hinstLibPtr, "ctlGetSupportedRetroScalingCapability");
        if (pfnGetSupportedRetroScalingCapability)
        {
            result = pfnGetSupportedRetroScalingCapability(hDAhandle, pRetroScalingCaps);
        }
    }

    return result;
}


/**
* @brief Get/Set Retro Scaling
* 
* @details
*     - Get or Set the status of retro scaling.This Api will do a physical
*       modeset resulting in flash on the screen
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pGetSetRetroScalingType`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSetRetroScaling(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to adapter
    ctl_retro_scaling_settings_t* pGetSetRetroScalingType   ///< [in,out][release] Get or Set the retro scaling type
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetRetroScaling_t pfnGetSetRetroScaling = (ctl_pfnGetSetRetroScaling_t)GetProcAddress(hinstLibPtr, "ctlGetSetRetroScaling");
        if (pfnGetSetRetroScaling)
        {
            result = pfnGetSetRetroScaling(hDAhandle, pGetSetRetroScalingType);
        }
    }

    return result;
}


/**
* @brief Get Supported Scaling Types
* 
* @details
*     - Returns supported scaling capabilities
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pScalingCaps`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSupportedScalingCapability(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_scaling_caps_t* pScalingCaps                ///< [in,out][release] Query result for supported scaling types
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSupportedScalingCapability_t pfnGetSupportedScalingCapability = (ctl_pfnGetSupportedScalingCapability_t)GetProcAddress(hinstLibPtr, "ctlGetSupportedScalingCapability");
        if (pfnGetSupportedScalingCapability)
        {
            result = pfnGetSupportedScalingCapability(hDisplayOutput, pScalingCaps);
        }
    }

    return result;
}


/**
* @brief Get Current Scaling
* 
* @details
*     - Returns current active scaling
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pGetCurrentScalingType`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetCurrentScaling(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_scaling_settings_t* pGetCurrentScalingType  ///< [in,out][release] Query result for active scaling types
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetCurrentScaling_t pfnGetCurrentScaling = (ctl_pfnGetCurrentScaling_t)GetProcAddress(hinstLibPtr, "ctlGetCurrentScaling");
        if (pfnGetCurrentScaling)
        {
            result = pfnGetCurrentScaling(hDisplayOutput, pGetCurrentScalingType);
        }
    }

    return result;
}


/**
* @brief Set Scaling Type
* 
* @details
*     - Returns current active scaling
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSetScalingType`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlSetCurrentScaling(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_scaling_settings_t* pSetScalingType         ///< [in,out][release] Set scaling types
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetCurrentScaling_t pfnSetCurrentScaling = (ctl_pfnSetCurrentScaling_t)GetProcAddress(hinstLibPtr, "ctlSetCurrentScaling");
        if (pfnSetCurrentScaling)
        {
            result = pfnSetCurrentScaling(hDisplayOutput, pSetScalingType);
        }
    }

    return result;
}


/**
* @brief Get LACE Config
* 
* @details
*     - Returns current LACE Config
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pLaceConfig`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_LACE_INVALID_DATA_ARGUMENT_PASSED - "Lace Incorrrect AggressivePercent data or LuxVsAggressive Map data passed by user"
*/
ctl_result_t CTL_APICALL
ctlGetLACEConfig(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_lace_config_t* pLaceConfig                  ///< [out]Lace configuration
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetLACEConfig_t pfnGetLACEConfig = (ctl_pfnGetLACEConfig_t)GetProcAddress(hinstLibPtr, "ctlGetLACEConfig");
        if (pfnGetLACEConfig)
        {
            result = pfnGetLACEConfig(hDisplayOutput, pLaceConfig);
        }
    }

    return result;
}


/**
* @brief Sets LACE Config
* 
* @details
*     - Sets LACE Config
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pLaceConfig`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_LACE_INVALID_DATA_ARGUMENT_PASSED - "Lace Incorrrect AggressivePercent data or LuxVsAggressive Map data passed by user"
*/
ctl_result_t CTL_APICALL
ctlSetLACEConfig(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in]Handle to display output
    ctl_lace_config_t* pLaceConfig                  ///< [in]Lace configuration
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetLACEConfig_t pfnSetLACEConfig = (ctl_pfnSetLACEConfig_t)GetProcAddress(hinstLibPtr, "ctlSetLACEConfig");
        if (pfnSetLACEConfig)
        {
            result = pfnSetLACEConfig(hDisplayOutput, pLaceConfig);
        }
    }

    return result;
}


/**
* @brief Get Software PSR caps/Set software PSR State
* 
* @details
*     - Returns Software PSR status or Sets Software PSR capabilities. This is
*       a reserved capability. By default, software PSR is not supported/will
*       not be enabled, need application to activate it, please contact Intel
*       for activation.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSoftwarePsrSetting`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlSoftwarePSR(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_sw_psr_settings_t* pSoftwarePsrSetting      ///< [in,out][release] Get Software PSR caps/state or Set Software PSR
                                                    ///< state
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSoftwarePSR_t pfnSoftwarePSR = (ctl_pfnSoftwarePSR_t)GetProcAddress(hinstLibPtr, "ctlSoftwarePSR");
        if (pfnSoftwarePSR)
        {
            result = pfnSoftwarePSR(hDisplayOutput, pSoftwarePsrSetting);
        }
    }

    return result;
}


/**
* @brief Get Intel Arc Sync information for monitor
* 
* @details
*     - Returns Intel Arc Sync information for selected monitor
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pIntelArcSyncMonitorParams`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetIntelArcSyncInfoForMonitor(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_intel_arc_sync_monitor_params_t* pIntelArcSyncMonitorParams ///< [in,out][release] Intel Arc Sync params for monitor
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetIntelArcSyncInfoForMonitor_t pfnGetIntelArcSyncInfoForMonitor = (ctl_pfnGetIntelArcSyncInfoForMonitor_t)GetProcAddress(hinstLibPtr, "ctlGetIntelArcSyncInfoForMonitor");
        if (pfnGetIntelArcSyncInfoForMonitor)
        {
            result = pfnGetIntelArcSyncInfoForMonitor(hDisplayOutput, pIntelArcSyncMonitorParams);
        }
    }

    return result;
}


/**
* @brief Get Intel Arc Sync profile
* 
* @details
*     - Returns Intel Arc Sync profile for selected monitor
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pIntelArcSyncProfileParams`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetIntelArcSyncProfile(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_intel_arc_sync_profile_params_t* pIntelArcSyncProfileParams ///< [in,out][release] Intel Arc Sync params for monitor
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetIntelArcSyncProfile_t pfnGetIntelArcSyncProfile = (ctl_pfnGetIntelArcSyncProfile_t)GetProcAddress(hinstLibPtr, "ctlGetIntelArcSyncProfile");
        if (pfnGetIntelArcSyncProfile)
        {
            result = pfnGetIntelArcSyncProfile(hDisplayOutput, pIntelArcSyncProfileParams);
        }
    }

    return result;
}


/**
* @brief Set Intel Arc Sync profile
* 
* @details
*     - Sets Intel Arc Sync profile for selected monitor. In a mux situation,
*       this API should be called for all display IDs associated with a
*       physical display.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pIntelArcSyncProfileParams`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlSetIntelArcSyncProfile(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_intel_arc_sync_profile_params_t* pIntelArcSyncProfileParams ///< [in][release] Intel Arc Sync params for monitor
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnSetIntelArcSyncProfile_t pfnSetIntelArcSyncProfile = (ctl_pfnSetIntelArcSyncProfile_t)GetProcAddress(hinstLibPtr, "ctlSetIntelArcSyncProfile");
        if (pfnSetIntelArcSyncProfile)
        {
            result = pfnSetIntelArcSyncProfile(hDisplayOutput, pIntelArcSyncProfileParams);
        }
    }

    return result;
}


/**
* @brief EDID Management allows managing an output's EDID or Plugged Status.
* 
* @details
*     - To manage output's EDID or Display ID. Supports native DP SST and HDMI
*       Display types.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pEdidManagementArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid combination of parameters"
*     - ::CTL_RESULT_ERROR_DISPLAY_NOT_ATTACHED - "Error for Output Device not attached"
*     - ::CTL_RESULT_ERROR_OUT_OF_DEVICE_MEMORY - "Insufficient device memory to satisfy call"
*     - ::CTL_RESULT_ERROR_DATA_NOT_FOUND - "Requested EDID data not present."
*/
ctl_result_t CTL_APICALL
ctlEdidManagement(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_edid_management_args_t* pEdidManagementArgs ///< [in,out] EDID management arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEdidManagement_t pfnEdidManagement = (ctl_pfnEdidManagement_t)GetProcAddress(hinstLibPtr, "ctlEdidManagement");
        if (pfnEdidManagement)
        {
            result = pfnEdidManagement(hDisplayOutput, pEdidManagementArgs);
        }
    }

    return result;
}


/**
* @brief Get/Set Custom mode.
* 
* @details
*     - To get or set custom mode.
*     - Add custom source mode operation supports only single mode additon at
*       a time.
*     - Remove custom source mode operation supports single or multiple mode
*       removal at a time.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCustomModeArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernal mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid combination of parameters"
*     - ::CTL_RESULT_ERROR_CUSTOM_MODE_STANDARD_CUSTOM_MODE_EXISTS - "Standard custom mode exists"
*     - ::CTL_RESULT_ERROR_CUSTOM_MODE_NON_CUSTOM_MATCHING_MODE_EXISTS - "Non custom matching mode exists"
*     - ::CTL_RESULT_ERROR_CUSTOM_MODE_INSUFFICIENT_MEMORY - "Custom mode insufficent memory"
*/
ctl_result_t CTL_APICALL
ctlGetSetCustomMode(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_get_set_custom_mode_args_t* pCustomModeArgs ///< [in,out] Custom mode arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetCustomMode_t pfnGetSetCustomMode = (ctl_pfnGetSetCustomMode_t)GetProcAddress(hinstLibPtr, "ctlGetSetCustomMode");
        if (pfnGetSetCustomMode)
        {
            result = pfnGetSetCustomMode(hDisplayOutput, pCustomModeArgs);
        }
    }

    return result;
}


/**
* @brief Get/Set Combined Display
* 
* @details
*     - To get or set combined display with given Child Targets on a Single
*       GPU or across identical GPUs. Multi-GPU(MGPU) combined display is
*       reserved i.e. it is not public and requires special application GUID.
*       MGPU Combined Display will get activated or deactivated in next boot.
*       MGPU scenario will internally link the associated adapters via Linked
*       Display Adapter Call, with supplied hDeviceAdapter being the LDA
*       Primary. If Genlock and enabled in Driver registry and supported by
*       given Display Config, MGPU Combined Display will enable MGPU Genlock
*       with supplied hDeviceAdapter being the Genlock Primary Adapter and the
*       First Child Display being the Primary Display.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCombinedDisplayArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*     - ::CTL_RESULT_ERROR_FEATURE_NOT_SUPPORTED - "Combined Display feature is not supported in this platform"
*     - ::CTL_RESULT_ERROR_ADAPTER_NOT_SUPPORTED_ON_LDA_SECONDARY - "Unsupported (secondary) adapter handle passed"
*/
ctl_result_t CTL_APICALL
ctlGetSetCombinedDisplay(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] Handle to control device adapter
    ctl_combined_display_args_t* pCombinedDisplayArgs   ///< [in,out] Setup and get combined display arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetCombinedDisplay_t pfnGetSetCombinedDisplay = (ctl_pfnGetSetCombinedDisplay_t)GetProcAddress(hinstLibPtr, "ctlGetSetCombinedDisplay");
        if (pfnGetSetCombinedDisplay)
        {
            result = pfnGetSetCombinedDisplay(hDeviceAdapter, pCombinedDisplayArgs);
        }
    }

    return result;
}


/**
* @brief Get/Set Display Genlock
* 
* @details
*     - To get or set Display Genlock.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == hDeviceAdapter`
*         + `nullptr == pGenlockArgs`
*         + `nullptr == hFailureDeviceAdapter`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_INVALID_SIZE - "Invalid topology structure size"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*/
ctl_result_t CTL_APICALL
ctlGetSetDisplayGenlock(
    ctl_device_adapter_handle_t* hDeviceAdapter,    ///< [in][release] Handle to control device adapter
    ctl_genlock_args_t* pGenlockArgs,               ///< [in,out] Display Genlock operation and information
    uint32_t AdapterCount,                          ///< [in] Number of device adapters
    ctl_device_adapter_handle_t* hFailureDeviceAdapter  ///< [out] Handle to address the failure device adapter in an error case
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetDisplayGenlock_t pfnGetSetDisplayGenlock = (ctl_pfnGetSetDisplayGenlock_t)GetProcAddress(hinstLibPtr, "ctlGetSetDisplayGenlock");
        if (pfnGetSetDisplayGenlock)
        {
            result = pfnGetSetDisplayGenlock(hDeviceAdapter, pGenlockArgs, AdapterCount, hFailureDeviceAdapter);
        }
    }

    return result;
}


/**
* @brief Get Vblank Timestamp
* 
* @details
*     - To get a list of vblank timestamps in microseconds for each child
*       target of a display.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pVblankTSArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS - "Insufficient permissions"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*/
ctl_result_t CTL_APICALL
ctlGetVblankTimestamp(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_vblank_ts_args_t* pVblankTSArgs             ///< [out] Get vblank timestamp arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetVblankTimestamp_t pfnGetVblankTimestamp = (ctl_pfnGetVblankTimestamp_t)GetProcAddress(hinstLibPtr, "ctlGetVblankTimestamp");
        if (pfnGetVblankTimestamp)
        {
            result = pfnGetVblankTimestamp(hDisplayOutput, pVblankTSArgs);
        }
    }

    return result;
}


/**
* @brief Link Display Adapters
* 
* @details
*     - To Link Display Adapters.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPrimaryAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pLdaArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*     - ::CTL_RESULT_ERROR_ADAPTER_ALREADY_LINKED - "Adapter is already linked"
*/
ctl_result_t CTL_APICALL
ctlLinkDisplayAdapters(
    ctl_device_adapter_handle_t hPrimaryAdapter,    ///< [in][release] Handle to Primary adapter in LDA chain
    ctl_lda_args_t* pLdaArgs                        ///< [in] Link Display Adapters Arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnLinkDisplayAdapters_t pfnLinkDisplayAdapters = (ctl_pfnLinkDisplayAdapters_t)GetProcAddress(hinstLibPtr, "ctlLinkDisplayAdapters");
        if (pfnLinkDisplayAdapters)
        {
            result = pfnLinkDisplayAdapters(hPrimaryAdapter, pLdaArgs);
        }
    }

    return result;
}


/**
* @brief Unlink Display Adapters
* 
* @details
*     - To Unlink Display Adapters
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPrimaryAdapter`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*     - ::CTL_RESULT_ERROR_ADAPTER_NOT_SUPPORTED_ON_LDA_SECONDARY - "Unsupported (secondary) adapter handle passed"
*/
ctl_result_t CTL_APICALL
ctlUnlinkDisplayAdapters(
    ctl_device_adapter_handle_t hPrimaryAdapter     ///< [in][release] Handle to Primary adapter in LDA chain
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnUnlinkDisplayAdapters_t pfnUnlinkDisplayAdapters = (ctl_pfnUnlinkDisplayAdapters_t)GetProcAddress(hinstLibPtr, "ctlUnlinkDisplayAdapters");
        if (pfnUnlinkDisplayAdapters)
        {
            result = pfnUnlinkDisplayAdapters(hPrimaryAdapter);
        }
    }

    return result;
}


/**
* @brief Get Linked Display Adapters
* 
* @details
*     - To return list of Linked Display Adapters.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPrimaryAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pLdaArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*     - ::CTL_RESULT_ERROR_ADAPTER_NOT_SUPPORTED_ON_LDA_SECONDARY - "Unsupported (secondary) adapter handle passed"
*/
ctl_result_t CTL_APICALL
ctlGetLinkedDisplayAdapters(
    ctl_device_adapter_handle_t hPrimaryAdapter,    ///< [in][release] Handle to Primary adapter in LDA chain
    ctl_lda_args_t* pLdaArgs                        ///< [out] Link Display Adapters Arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetLinkedDisplayAdapters_t pfnGetLinkedDisplayAdapters = (ctl_pfnGetLinkedDisplayAdapters_t)GetProcAddress(hinstLibPtr, "ctlGetLinkedDisplayAdapters");
        if (pfnGetLinkedDisplayAdapters)
        {
            result = pfnGetLinkedDisplayAdapters(hPrimaryAdapter, pLdaArgs);
        }
    }

    return result;
}


/**
* @brief Get/Set Dynamic Contrast Enhancement
* 
* @details
*     - To get the DCE feature status and, if feature is enabled, returns the
*       current histogram, or to set the brightness at the phase-in speed
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pDceArgs`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_HANDLE - "Invalid or Null handle passed"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid combination of parameters"
*/
ctl_result_t CTL_APICALL
ctlGetSetDynamicContrastEnhancement(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_dce_args_t* pDceArgs                        ///< [in,out] Dynamic Contrast Enhancement arguments
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetDynamicContrastEnhancement_t pfnGetSetDynamicContrastEnhancement = (ctl_pfnGetSetDynamicContrastEnhancement_t)GetProcAddress(hinstLibPtr, "ctlGetSetDynamicContrastEnhancement");
        if (pfnGetSetDynamicContrastEnhancement)
        {
            result = pfnGetSetDynamicContrastEnhancement(hDisplayOutput, pDceArgs);
        }
    }

    return result;
}


/**
* @brief Get/Set Color Format and Color Depth
* 
* @details
*     - Get and Set the Color Format and Color Depth of a target
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pGetSetWireFormatSetting`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid data passed as argument, WireFormat is not supported"
*     - ::CTL_RESULT_ERROR_DISPLAY_NOT_ACTIVE - "Display not active"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*/
ctl_result_t CTL_APICALL
ctlGetSetWireFormat(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in][release] Handle to display output
    ctl_get_set_wire_format_config_t* pGetSetWireFormatSetting  ///< [in][release] Get/Set Wire Format settings to be fetched/applied
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetWireFormat_t pfnGetSetWireFormat = (ctl_pfnGetSetWireFormat_t)GetProcAddress(hinstLibPtr, "ctlGetSetWireFormat");
        if (pfnGetSetWireFormat)
        {
            result = pfnGetSetWireFormat(hDisplayOutput, pGetSetWireFormatSetting);
        }
    }

    return result;
}


/**
* @brief Get/Set Display settings
* 
* @details
*     - To get/set end display settings like low latency, HDR10+ signaling
*       etc. which are controlled via info-frames/secondary data packets
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDisplayOutput`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pDisplaySettings`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_NULL_OS_DISPLAY_OUTPUT_HANDLE - "Null OS display output handle"
*     - ::CTL_RESULT_ERROR_NULL_OS_INTERFACE - "Null OS interface"
*     - ::CTL_RESULT_ERROR_NULL_OS_ADAPATER_HANDLE - "Null OS adapter handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "Kernel mode driver call failure"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_HANDLE - "Invalid or Null handle passed"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_POINTER - "Invalid null pointer"
*     - ::CTL_RESULT_ERROR_INVALID_OPERATION_TYPE - "Invalid operation type"
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT - "Invalid combination of parameters"
*/
ctl_result_t CTL_APICALL
ctlGetSetDisplaySettings(
    ctl_display_output_handle_t hDisplayOutput,     ///< [in] Handle to display output
    ctl_display_settings_t* pDisplaySettings        ///< [in,out] End display capabilities
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetDisplaySettings_t pfnGetSetDisplaySettings = (ctl_pfnGetSetDisplaySettings_t)GetProcAddress(hinstLibPtr, "ctlGetSetDisplaySettings");
        if (pfnGetSetDisplaySettings)
        {
            result = pfnGetSetDisplaySettings(hDisplayOutput, pDisplaySettings);
        }
    }

    return result;
}


/**
* @brief Get ECC properties.
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlEccGetProperties(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_ecc_properties_t* pProperties               ///< [in,out] Will contain ECC properties.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEccGetProperties_t pfnEccGetProperties = (ctl_pfnEccGetProperties_t)GetProcAddress(hinstLibPtr, "ctlEccGetProperties");
        if (pfnEccGetProperties)
        {
            result = pfnEccGetProperties(hDAhandle, pProperties);
        }
    }

    return result;
}


/**
* @brief Get ECC state.
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*     - CTL_RESULT_ERROR_INVALID_ENUMERATION
*         + `::CTL_ECC_STATE_ECC_DISABLED_STATE < pState->currentEccState`
*         + `::CTL_ECC_STATE_ECC_DISABLED_STATE < pState->pendingEccState`
*/
ctl_result_t CTL_APICALL
ctlEccGetState(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_ecc_state_desc_t* pState                    ///< [in,out] Will contain the current ECC state and pending ECC state to
                                                    ///< be applied from previous ctlEccSetState() call.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEccGetState_t pfnEccGetState = (ctl_pfnEccGetState_t)GetProcAddress(hinstLibPtr, "ctlEccGetState");
        if (pfnEccGetState)
        {
            result = pfnEccGetState(hDAhandle, pState);
        }
    }

    return result;
}


/**
* @brief Set ECC state. Setting CTL_ECC_STATE_ECC_DEFAULT_STATE will reset the
*        ECC state to the factory settings.
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*     - CTL_RESULT_ERROR_INVALID_ENUMERATION
*         + `::CTL_ECC_STATE_ECC_DISABLED_STATE < pState->currentEccState`
*         + `::CTL_ECC_STATE_ECC_DISABLED_STATE < pState->pendingEccState`
*/
ctl_result_t CTL_APICALL
ctlEccSetState(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_ecc_state_desc_t* pState                    ///< [in,out] Will contain the new ECC state and pending ECC state from
                                                    ///< ctlEccSetState() call.
                                                    ///< New ECC State can be set only if isSupported is true and canControl is true.
                                                    ///< ctlEccGetState() can be called to determine if the currentEccState is
                                                    ///< not equal to pendingEccState, then system reboot is needed for the
                                                    ///< pendingEccState to be applied.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEccSetState_t pfnEccSetState = (ctl_pfnEccSetState_t)GetProcAddress(hinstLibPtr, "ctlEccSetState");
        if (pfnEccSetState)
        {
            result = pfnEccSetState(hDAhandle, pState);
        }
    }

    return result;
}


/**
* @brief Get handle of engine groups
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumEngineGroups(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_engine_handle_t* phEngine                   ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< if count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumEngineGroups_t pfnEnumEngineGroups = (ctl_pfnEnumEngineGroups_t)GetProcAddress(hinstLibPtr, "ctlEnumEngineGroups");
        if (pfnEnumEngineGroups)
        {
            result = pfnEnumEngineGroups(hDAhandle, pCount, phEngine);
        }
    }

    return result;
}


/**
* @brief Get engine group properties
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hEngine`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlEngineGetProperties(
    ctl_engine_handle_t hEngine,                    ///< [in] Handle for the component.
    ctl_engine_properties_t* pProperties            ///< [in,out] The properties for the specified engine group.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEngineGetProperties_t pfnEngineGetProperties = (ctl_pfnEngineGetProperties_t)GetProcAddress(hinstLibPtr, "ctlEngineGetProperties");
        if (pfnEngineGetProperties)
        {
            result = pfnEngineGetProperties(hEngine, pProperties);
        }
    }

    return result;
}


/**
* @brief Get the activity stats for an engine group
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hEngine`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pStats`
*/
ctl_result_t CTL_APICALL
ctlEngineGetActivity(
    ctl_engine_handle_t hEngine,                    ///< [in] Handle for the component.
    ctl_engine_stats_t* pStats                      ///< [in,out] Will contain a snapshot of the engine group activity
                                                    ///< counters.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEngineGetActivity_t pfnEngineGetActivity = (ctl_pfnEngineGetActivity_t)GetProcAddress(hinstLibPtr, "ctlEngineGetActivity");
        if (pfnEngineGetActivity)
        {
            result = pfnEngineGetActivity(hEngine, pStats);
        }
    }

    return result;
}


/**
* @brief Get handle of fans
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumFans(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to the adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_fan_handle_t* phFan                         ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< if count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumFans_t pfnEnumFans = (ctl_pfnEnumFans_t)GetProcAddress(hinstLibPtr, "ctlEnumFans");
        if (pfnEnumFans)
        {
            result = pfnEnumFans(hDAhandle, pCount, phFan);
        }
    }

    return result;
}


/**
* @brief Get fan properties
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFan`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlFanGetProperties(
    ctl_fan_handle_t hFan,                          ///< [in] Handle for the component.
    ctl_fan_properties_t* pProperties               ///< [in,out] Will contain the properties of the fan.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFanGetProperties_t pfnFanGetProperties = (ctl_pfnFanGetProperties_t)GetProcAddress(hinstLibPtr, "ctlFanGetProperties");
        if (pfnFanGetProperties)
        {
            result = pfnFanGetProperties(hFan, pProperties);
        }
    }

    return result;
}


/**
* @brief Get fan configurations and the current fan speed mode (default, fixed,
*        temp-speed table)
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFan`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pConfig`
*/
ctl_result_t CTL_APICALL
ctlFanGetConfig(
    ctl_fan_handle_t hFan,                          ///< [in] Handle for the component.
    ctl_fan_config_t* pConfig                       ///< [in,out] Will contain the current configuration of the fan.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFanGetConfig_t pfnFanGetConfig = (ctl_pfnFanGetConfig_t)GetProcAddress(hinstLibPtr, "ctlFanGetConfig");
        if (pfnFanGetConfig)
        {
            result = pfnFanGetConfig(hFan, pConfig);
        }
    }

    return result;
}


/**
* @brief Configure the fan to run with hardware factory settings (set mode to
*        ::CTL_FAN_SPEED_MODE_DEFAULT)
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFan`
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS
*         + User does not have permissions to make these modifications.
*/
ctl_result_t CTL_APICALL
ctlFanSetDefaultMode(
    ctl_fan_handle_t hFan                           ///< [in] Handle for the component.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFanSetDefaultMode_t pfnFanSetDefaultMode = (ctl_pfnFanSetDefaultMode_t)GetProcAddress(hinstLibPtr, "ctlFanSetDefaultMode");
        if (pfnFanSetDefaultMode)
        {
            result = pfnFanSetDefaultMode(hFan);
        }
    }

    return result;
}


/**
* @brief Configure the fan to rotate at a fixed speed (set mode to
*        ::CTL_FAN_SPEED_MODE_FIXED)
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFan`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == speed`
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS
*         + User does not have permissions to make these modifications.
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_FEATURE
*         + Fixing the fan speed not supported by the hardware or the fan speed units are not supported. See ::ctl_fan_properties_t.supportedModes and ::ctl_fan_properties_t.supportedUnits.
*/
ctl_result_t CTL_APICALL
ctlFanSetFixedSpeedMode(
    ctl_fan_handle_t hFan,                          ///< [in] Handle for the component.
    const ctl_fan_speed_t* speed                    ///< [in] The fixed fan speed setting
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFanSetFixedSpeedMode_t pfnFanSetFixedSpeedMode = (ctl_pfnFanSetFixedSpeedMode_t)GetProcAddress(hinstLibPtr, "ctlFanSetFixedSpeedMode");
        if (pfnFanSetFixedSpeedMode)
        {
            result = pfnFanSetFixedSpeedMode(hFan, speed);
        }
    }

    return result;
}


/**
* @brief Configure the fan to adjust speed based on a temperature/speed table
*        (set mode to ::CTL_FAN_SPEED_MODE_TABLE)
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFan`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == speedTable`
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS
*         + User does not have permissions to make these modifications.
*     - ::CTL_RESULT_ERROR_INVALID_ARGUMENT
*         + The temperature/speed pairs in the array are not sorted on temperature from lowest to highest.
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_FEATURE
*         + Fan speed table not supported by the hardware or the fan speed units are not supported. See ::ctl_fan_properties_t.supportedModes and ::ctl_fan_properties_t.supportedUnits.
*/
ctl_result_t CTL_APICALL
ctlFanSetSpeedTableMode(
    ctl_fan_handle_t hFan,                          ///< [in] Handle for the component.
    const ctl_fan_speed_table_t* speedTable         ///< [in] A table containing temperature/speed pairs.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFanSetSpeedTableMode_t pfnFanSetSpeedTableMode = (ctl_pfnFanSetSpeedTableMode_t)GetProcAddress(hinstLibPtr, "ctlFanSetSpeedTableMode");
        if (pfnFanSetSpeedTableMode)
        {
            result = pfnFanSetSpeedTableMode(hFan, speedTable);
        }
    }

    return result;
}


/**
* @brief Get current state of a fan - current mode and speed
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFan`
*     - CTL_RESULT_ERROR_INVALID_ENUMERATION
*         + `::CTL_FAN_SPEED_UNITS_PERCENT < units`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSpeed`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_FEATURE
*         + The requested fan speed units are not supported. See ::ctl_fan_properties_t.supportedUnits.
*/
ctl_result_t CTL_APICALL
ctlFanGetState(
    ctl_fan_handle_t hFan,                          ///< [in] Handle for the component.
    ctl_fan_speed_units_t units,                    ///< [in] The units in which the fan speed should be returned.
    int32_t* pSpeed                                 ///< [in,out] Will contain the current speed of the fan in the units
                                                    ///< requested. A value of -1 indicates that the fan speed cannot be
                                                    ///< measured.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFanGetState_t pfnFanGetState = (ctl_pfnFanGetState_t)GetProcAddress(hinstLibPtr, "ctlFanGetState");
        if (pfnFanGetState)
        {
            result = pfnFanGetState(hFan, units, pSpeed);
        }
    }

    return result;
}


/**
* @brief Get base firmware properties
* 
* @details
*     - The application gets properties of base firmware
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetFirmwareProperties(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    ctl_firmware_properties_t* pProperties          ///< [in,out] Pointer to an array that will hold properties of the base
                                                    ///< firmware.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetFirmwareProperties_t pfnGetFirmwareProperties = (ctl_pfnGetFirmwareProperties_t)GetProcAddress(hinstLibPtr, "ctlGetFirmwareProperties");
        if (pfnGetFirmwareProperties)
        {
            result = pfnGetFirmwareProperties(hDeviceAdapter, pProperties);
        }
    }

    return result;
}


/**
* @brief Get handle of various firmware components
* 
* @details
*     - The application enumerates all firmware components on an Intel
*       Discrete Graphics device.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_HANDLE - "Invalid handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "KMD call failed"
*/
ctl_result_t CTL_APICALL
ctlEnumerateFirmwareComponents(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_firmware_component_handle_t* phFirmware     ///< [in,out][optional][release][range(0, *pCount)] array of handle of
                                                    ///< firmware components.
                                                    ///< If count is less than the number of firmware components that are
                                                    ///< available, then the driver shall only retrieve that number of firmware
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumerateFirmwareComponents_t pfnEnumerateFirmwareComponents = (ctl_pfnEnumerateFirmwareComponents_t)GetProcAddress(hinstLibPtr, "ctlEnumerateFirmwareComponents");
        if (pfnEnumerateFirmwareComponents)
        {
            result = pfnEnumerateFirmwareComponents(hDeviceAdapter, pCount, phFirmware);
        }
    }

    return result;
}


/**
* @brief Get firmware component properties
* 
* @details
*     - The application gets properties of individual firmware components
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFirmware`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_HANDLE - "Invalid handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "KMD call failed"
*/
ctl_result_t CTL_APICALL
ctlGetFirmwareComponentProperties(
    ctl_firmware_component_handle_t hFirmware,      ///< [in] Handle for the firmware component.
    ctl_firmware_component_properties_t* pProperties///< [in,out] Pointer to an array that will hold properties of the firmware
                                                    ///< component.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetFirmwareComponentProperties_t pfnGetFirmwareComponentProperties = (ctl_pfnGetFirmwareComponentProperties_t)GetProcAddress(hinstLibPtr, "ctlGetFirmwareComponentProperties");
        if (pfnGetFirmwareComponentProperties)
        {
            result = pfnGetFirmwareComponentProperties(hFirmware, pProperties);
        }
    }

    return result;
}


/**
* @brief Allows/Blocks discrete graphics device firmware's capability to train
*        PCI-E link at higher speeds on compatible compatible hosts
* 
* @details
*     - This API allows caller to allow/block a compatible discrete graphics
*       card's firmware train PCIE links at higher speeds on compatible hosts.
*     - System needs to be powered off and restarted for the new state to take
*       affect. The new state will not be applied on only a warm reboot of the
*       system.
*     - This is a reserved capability. By default, this capability will not be
*       enabled, need application to activate it, please contact Intel for
*       activation.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*     - ::CTL_RESULT_ERROR_INVALID_NULL_HANDLE - "Invalid handle"
*     - ::CTL_RESULT_ERROR_KMD_CALL - "KMD call failed"
*/
ctl_result_t CTL_APICALL
ctlAllowPCIeLinkSpeedUpdate(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] handle to control device adapter
    bool AllowPCIeLinkSpeedUpdate                   ///< [in] When set configures the device firmware to train PCI-E link at
                                                    ///< higher speeds, else this will block the device firmware from training
                                                    ///< at higher PCI-E link speeds on compatible hosts.
                                                    ///< This API modifies a flash persistant setting of the device firmware to
                                                    ///< allow/block training PCI-E link at higher speeds.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnAllowPCIeLinkSpeedUpdate_t pfnAllowPCIeLinkSpeedUpdate = (ctl_pfnAllowPCIeLinkSpeedUpdate_t)GetProcAddress(hinstLibPtr, "ctlAllowPCIeLinkSpeedUpdate");
        if (pfnAllowPCIeLinkSpeedUpdate)
        {
            result = pfnAllowPCIeLinkSpeedUpdate(hDeviceAdapter, AllowPCIeLinkSpeedUpdate);
        }
    }

    return result;
}


/**
* @brief Get handle of frequency domains
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumFrequencyDomains(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_freq_handle_t* phFrequency                  ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< if count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumFrequencyDomains_t pfnEnumFrequencyDomains = (ctl_pfnEnumFrequencyDomains_t)GetProcAddress(hinstLibPtr, "ctlEnumFrequencyDomains");
        if (pfnEnumFrequencyDomains)
        {
            result = pfnEnumFrequencyDomains(hDAhandle, pCount, phFrequency);
        }
    }

    return result;
}


/**
* @brief Get frequency properties - available frequencies
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFrequency`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlFrequencyGetProperties(
    ctl_freq_handle_t hFrequency,                   ///< [in] Handle for the component.
    ctl_freq_properties_t* pProperties              ///< [in,out] The frequency properties for the specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFrequencyGetProperties_t pfnFrequencyGetProperties = (ctl_pfnFrequencyGetProperties_t)GetProcAddress(hinstLibPtr, "ctlFrequencyGetProperties");
        if (pfnFrequencyGetProperties)
        {
            result = pfnFrequencyGetProperties(hFrequency, pProperties);
        }
    }

    return result;
}


/**
* @brief Get available non-overclocked hardware clock frequencies for the
*        frequency domain
* 
* @details
*     - The list of available frequencies is returned in order of slowest to
*       fastest.
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFrequency`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlFrequencyGetAvailableClocks(
    ctl_freq_handle_t hFrequency,                   ///< [in] Device handle of the device.
    uint32_t* pCount,                               ///< [in,out] pointer to the number of frequencies.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of frequencies that are available.
                                                    ///< if count is greater than the number of frequencies that are available,
                                                    ///< then the driver shall update the value with the correct number of frequencies.
    double* phFrequency                             ///< [in,out][optional][range(0, *pCount)] array of frequencies in units of
                                                    ///< MHz and sorted from slowest to fastest.
                                                    ///< if count is less than the number of frequencies that are available,
                                                    ///< then the driver shall only retrieve that number of frequencies.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFrequencyGetAvailableClocks_t pfnFrequencyGetAvailableClocks = (ctl_pfnFrequencyGetAvailableClocks_t)GetProcAddress(hinstLibPtr, "ctlFrequencyGetAvailableClocks");
        if (pfnFrequencyGetAvailableClocks)
        {
            result = pfnFrequencyGetAvailableClocks(hFrequency, pCount, phFrequency);
        }
    }

    return result;
}


/**
* @brief Get current frequency limits
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFrequency`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pLimits`
*/
ctl_result_t CTL_APICALL
ctlFrequencyGetRange(
    ctl_freq_handle_t hFrequency,                   ///< [in] Handle for the component.
    ctl_freq_range_t* pLimits                       ///< [in,out] The range between which the hardware can operate for the
                                                    ///< specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFrequencyGetRange_t pfnFrequencyGetRange = (ctl_pfnFrequencyGetRange_t)GetProcAddress(hinstLibPtr, "ctlFrequencyGetRange");
        if (pfnFrequencyGetRange)
        {
            result = pfnFrequencyGetRange(hFrequency, pLimits);
        }
    }

    return result;
}


/**
* @brief Set frequency range between which the hardware can operate.
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFrequency`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pLimits`
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS
*         + User does not have permissions to make these modifications.
*/
ctl_result_t CTL_APICALL
ctlFrequencySetRange(
    ctl_freq_handle_t hFrequency,                   ///< [in] Handle for the component.
    const ctl_freq_range_t* pLimits                 ///< [in] The limits between which the hardware can operate for the
                                                    ///< specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFrequencySetRange_t pfnFrequencySetRange = (ctl_pfnFrequencySetRange_t)GetProcAddress(hinstLibPtr, "ctlFrequencySetRange");
        if (pfnFrequencySetRange)
        {
            result = pfnFrequencySetRange(hFrequency, pLimits);
        }
    }

    return result;
}


/**
* @brief Get current frequency state - frequency request, actual frequency, TDP
*        limits
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFrequency`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*/
ctl_result_t CTL_APICALL
ctlFrequencyGetState(
    ctl_freq_handle_t hFrequency,                   ///< [in] Handle for the component.
    ctl_freq_state_t* pState                        ///< [in,out] Frequency state for the specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFrequencyGetState_t pfnFrequencyGetState = (ctl_pfnFrequencyGetState_t)GetProcAddress(hinstLibPtr, "ctlFrequencyGetState");
        if (pfnFrequencyGetState)
        {
            result = pfnFrequencyGetState(hFrequency, pState);
        }
    }

    return result;
}


/**
* @brief Get frequency throttle time
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hFrequency`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pThrottleTime`
*/
ctl_result_t CTL_APICALL
ctlFrequencyGetThrottleTime(
    ctl_freq_handle_t hFrequency,                   ///< [in] Handle for the component.
    ctl_freq_throttle_time_t* pThrottleTime         ///< [in,out] Will contain a snapshot of the throttle time counters for the
                                                    ///< specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnFrequencyGetThrottleTime_t pfnFrequencyGetThrottleTime = (ctl_pfnFrequencyGetThrottleTime_t)GetProcAddress(hinstLibPtr, "ctlFrequencyGetThrottleTime");
        if (pfnFrequencyGetThrottleTime)
        {
            result = pfnFrequencyGetThrottleTime(hFrequency, pThrottleTime);
        }
    }

    return result;
}


/**
* @brief Get handle of Leds
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumLeds(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< If count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< If count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_led_handle_t* phLed                         ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< If count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumLeds_t pfnEnumLeds = (ctl_pfnEnumLeds_t)GetProcAddress(hinstLibPtr, "ctlEnumLeds");
        if (pfnEnumLeds)
        {
            result = pfnEnumLeds(hDAhandle, pCount, phLed);
        }
    }

    return result;
}


/**
* @brief Get Led properties
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hLed`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlLedGetProperties(
    ctl_led_handle_t hLed,                          ///< [in] Handle for the component.
    ctl_led_properties_t* pProperties               ///< [in,out] Will contain Led properties.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnLedGetProperties_t pfnLedGetProperties = (ctl_pfnLedGetProperties_t)GetProcAddress(hinstLibPtr, "ctlLedGetProperties");
        if (pfnLedGetProperties)
        {
            result = pfnLedGetProperties(hLed, pProperties);
        }
    }

    return result;
}


/**
* @brief Get Led state
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hLed`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*/
ctl_result_t CTL_APICALL
ctlLedGetState(
    ctl_led_handle_t hLed,                          ///< [in] Handle for the component.
    ctl_led_state_t* pState                         ///< [in,out] Will contain the current Led state.
                                                    ///< Returns Led state if canControl is true and isI2C is false.
                                                    ///< pwm and color structure members of ::ctl_led_state_t will be returned
                                                    ///< only if supported by Led, else they will be returned as 0.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnLedGetState_t pfnLedGetState = (ctl_pfnLedGetState_t)GetProcAddress(hinstLibPtr, "ctlLedGetState");
        if (pfnLedGetState)
        {
            result = pfnLedGetState(hLed, pState);
        }
    }

    return result;
}


/**
* @brief Set Led state
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
*     - This API is rate-limited by 500 milliseconds, If this API is called
*       too frequently ::CTL_ERROR_CORE_LED_TOO_FREQUENT_SET_REQUESTS error
*       will be returned
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hLed`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pBuffer`
*/
ctl_result_t CTL_APICALL
ctlLedSetState(
    ctl_led_handle_t hLed,                          ///< [in] Handle for the component.
    void* pBuffer,                                  ///< [in] Led State buffer.
                                                    ///< If isI2C is true, the pBuffer and bufferSize will be passed to the I2C
                                                    ///< Interface. pBuffer format in this case is OEM defined.
                                                    ///< If isI2C is false, the pBuffer will be typecasted to
                                                    ///< ::ctl_led_state_t* and bufferSize needs to be sizeof
                                                    ///< ::ctl_led_state_t. pwm and color structure members of
                                                    ///< ::ctl_led_state_t will be set only if supported by Led, else they will
                                                    ///< be ignored.
    uint32_t bufferSize                             ///< [in] Led State buffer size.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnLedSetState_t pfnLedSetState = (ctl_pfnLedSetState_t)GetProcAddress(hinstLibPtr, "ctlLedSetState");
        if (pfnLedSetState)
        {
            result = pfnLedSetState(hLed, pBuffer, bufferSize);
        }
    }

    return result;
}


/**
* @brief Get Video Processing capabilities
* 
* @details
*     - The application gets Video Processing properties
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pFeatureCaps`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSupportedVideoProcessingCapabilities(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_video_processing_feature_caps_t* pFeatureCaps   ///< [in,out][release] Video Processing properties
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSupportedVideoProcessingCapabilities_t pfnGetSupportedVideoProcessingCapabilities = (ctl_pfnGetSupportedVideoProcessingCapabilities_t)GetProcAddress(hinstLibPtr, "ctlGetSupportedVideoProcessingCapabilities");
        if (pfnGetSupportedVideoProcessingCapabilities)
        {
            result = pfnGetSupportedVideoProcessingCapabilities(hDAhandle, pFeatureCaps);
        }
    }

    return result;
}


/**
* @brief Get/Set Video Processing feature details
* 
* @details
*     - Video Processing feature details
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pFeature`
*     - ::CTL_RESULT_ERROR_UNSUPPORTED_VERSION - "Unsupported version"
*/
ctl_result_t CTL_APICALL
ctlGetSetVideoProcessingFeature(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_video_processing_feature_getset_t* pFeature ///< [in][release] Video Processing feature get/set parameter
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnGetSetVideoProcessingFeature_t pfnGetSetVideoProcessingFeature = (ctl_pfnGetSetVideoProcessingFeature_t)GetProcAddress(hinstLibPtr, "ctlGetSetVideoProcessingFeature");
        if (pfnGetSetVideoProcessingFeature)
        {
            result = pfnGetSetVideoProcessingFeature(hDAhandle, pFeature);
        }
    }

    return result;
}


/**
* @brief Get handle of memory modules
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumMemoryModules(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_mem_handle_t* phMemory                      ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< if count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumMemoryModules_t pfnEnumMemoryModules = (ctl_pfnEnumMemoryModules_t)GetProcAddress(hinstLibPtr, "ctlEnumMemoryModules");
        if (pfnEnumMemoryModules)
        {
            result = pfnEnumMemoryModules(hDAhandle, pCount, phMemory);
        }
    }

    return result;
}


/**
* @brief Get memory properties
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hMemory`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlMemoryGetProperties(
    ctl_mem_handle_t hMemory,                       ///< [in] Handle for the component.
    ctl_mem_properties_t* pProperties               ///< [in,out] Will contain memory properties.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnMemoryGetProperties_t pfnMemoryGetProperties = (ctl_pfnMemoryGetProperties_t)GetProcAddress(hinstLibPtr, "ctlMemoryGetProperties");
        if (pfnMemoryGetProperties)
        {
            result = pfnMemoryGetProperties(hMemory, pProperties);
        }
    }

    return result;
}


/**
* @brief Get memory state - health, allocated
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hMemory`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*/
ctl_result_t CTL_APICALL
ctlMemoryGetState(
    ctl_mem_handle_t hMemory,                       ///< [in] Handle for the component.
    ctl_mem_state_t* pState                         ///< [in,out] Will contain the current health and allocated memory.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnMemoryGetState_t pfnMemoryGetState = (ctl_pfnMemoryGetState_t)GetProcAddress(hinstLibPtr, "ctlMemoryGetState");
        if (pfnMemoryGetState)
        {
            result = pfnMemoryGetState(hMemory, pState);
        }
    }

    return result;
}


/**
* @brief Get memory bandwidth
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hMemory`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pBandwidth`
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS
*         + User does not have permissions to query this telemetry.
*/
ctl_result_t CTL_APICALL
ctlMemoryGetBandwidth(
    ctl_mem_handle_t hMemory,                       ///< [in] Handle for the component.
    ctl_mem_bandwidth_t* pBandwidth                 ///< [in,out] Will contain the current health, free memory, total memory
                                                    ///< size.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnMemoryGetBandwidth_t pfnMemoryGetBandwidth = (ctl_pfnMemoryGetBandwidth_t)GetProcAddress(hinstLibPtr, "ctlMemoryGetBandwidth");
        if (pfnMemoryGetBandwidth)
        {
            result = pfnMemoryGetBandwidth(hMemory, pBandwidth);
        }
    }

    return result;
}


/**
* @brief Get overclock properties - available properties.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcProperties`
*/
ctl_result_t CTL_APICALL
ctlOverclockGetProperties(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    ctl_oc_properties_t* pOcProperties              ///< [in,out] The overclocking properties for the specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGetProperties_t pfnOverclockGetProperties = (ctl_pfnOverclockGetProperties_t)GetProcAddress(hinstLibPtr, "ctlOverclockGetProperties");
        if (pfnOverclockGetProperties)
        {
            result = pfnOverclockGetProperties(hDeviceHandle, pOcProperties);
        }
    }

    return result;
}


/**
* @brief Overclock Waiver - Warranty Waiver.
* 
* @details
*     - Most of the overclock functions will return an error if the waiver is
*       not set. This is because most overclock settings will increase the
*       electric/thermal stress on the part and thus reduce its lifetime.
*     - By setting the waiver, the user is indicate that they are accepting a
*       reduction in the lifetime of the part.
*     - It is the responsibility of overclock applications to notify each user
*       at least once with a popup of the dangers and requiring acceptance.
*     - Only once the user has accepted should this function be called by the
*       application.
*     - It is acceptable for the application to cache the user choice and call
*       this function on future executions without issuing the popup.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockWaiverSet(
    ctl_device_adapter_handle_t hDeviceHandle       ///< [in][release] Handle to display adapter
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockWaiverSet_t pfnOverclockWaiverSet = (ctl_pfnOverclockWaiverSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockWaiverSet");
        if (pfnOverclockWaiverSet)
        {
            result = pfnOverclockWaiverSet(hDeviceHandle);
        }
    }

    return result;
}


/**
* @brief Get the Overclock Frequency Offset for the GPU in MHz.
* 
* @details
*     - Determine the current frequency offset in effect (refer to
*       ::ctlOverclockGpuFrequencyOffsetSet() for details).
*     - The value returned may be different from the value that was previously
*       set by the application depending on hardware limitations or if the
*       function ::ctlOverclockGpuFrequencyOffsetSet() has been called or
*       another application that has changed the value.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcFrequencyOffset`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuFrequencyOffsetGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pOcFrequencyOffset                      ///< [in,out] The Turbo Overclocking Frequency Desired in MHz.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuFrequencyOffsetGet_t pfnOverclockGpuFrequencyOffsetGet = (ctl_pfnOverclockGpuFrequencyOffsetGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuFrequencyOffsetGet");
        if (pfnOverclockGpuFrequencyOffsetGet)
        {
            result = pfnOverclockGpuFrequencyOffsetGet(hDeviceHandle, pOcFrequencyOffset);
        }
    }

    return result;
}


/**
* @brief Set the Overclock Frequency Offset for the GPU in MHZ.
* 
* @details
*     - The purpose of this function is to increase/decrease the frequency at
*       which typical workloads will run within the same thermal budget.
*     - The frequency offset is expressed in units of ±1MHz.
*     - The actual operating frequency for each workload is not guaranteed to
*       change exactly by the specified offset.
*     - For positive frequency offsets, the factory maximum frequency may
*       increase by up to the specified amount.
*     - For negative frequency offsets, the overclock waiver must have been
*       set since this can result in running the part at voltages beyond the
*       part warrantee limits. An error is returned if the waiver has not been
*       set.
*     - Specifying large values for the frequency offset can lead to
*       instability. It is recommended that changes are made in small
*       increments and stability/performance measured running intense GPU
*       workloads before increasing further.
*     - This setting is not persistent through system reboots or driver
*       resets/hangs. It is up to the overclock application to reapply the
*       settings in those cases.
*     - This setting can cause system/device instability. It is up to the
*       overclock application to detect if the system has rebooted
*       unexpectedly or the device was restarted. When this occurs, the
*       application should not reapply the overclock settings automatically
*       but instead return to previously known good settings or notify the
*       user that the settings are not being applied.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuFrequencyOffsetSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double ocFrequencyOffset                        ///< [in] The Turbo Overclocking Frequency Desired in MHz.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuFrequencyOffsetSet_t pfnOverclockGpuFrequencyOffsetSet = (ctl_pfnOverclockGpuFrequencyOffsetSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuFrequencyOffsetSet");
        if (pfnOverclockGpuFrequencyOffsetSet)
        {
            result = pfnOverclockGpuFrequencyOffsetSet(hDeviceHandle, ocFrequencyOffset);
        }
    }

    return result;
}


/**
* @brief Get the Overclock Gpu Voltage Offset in mV.
* 
* @details
*     - Determine the current voltage offset in effect on the hardware (refer
*       to ::ctlOverclockGpuVoltageOffsetSet for details).
*     - The value returned may be different from the value that was previously
*       set by the application depending on hardware limitations or if the
*       function ::ctlOverclockGpuVoltageOffsetSet has been called or another
*       application that has changed the value.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcVoltageOffset`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuVoltageOffsetGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pOcVoltageOffset                        ///< [in,out] The Turbo Overclocking Frequency Desired in mV.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuVoltageOffsetGet_t pfnOverclockGpuVoltageOffsetGet = (ctl_pfnOverclockGpuVoltageOffsetGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuVoltageOffsetGet");
        if (pfnOverclockGpuVoltageOffsetGet)
        {
            result = pfnOverclockGpuVoltageOffsetGet(hDeviceHandle, pOcVoltageOffset);
        }
    }

    return result;
}


/**
* @brief Set the Overclock Gpu Voltage Offset in mV.
* 
* @details
*     - The purpose of this function is to attempt to run the GPU up to higher
*       voltages beyond the part warrantee limits. This can permit running at
*       even higher frequencies than can be obtained using the frequency
*       offset setting, but at the risk of reducing the lifetime of the part.
*     - The voltage offset is expressed in units of ±millivolts with values
*       permitted down to a resolution of 1 millivolt.
*     - The overclock waiver must be set before calling this function
*       otherwise and error will be returned.
*     - There is no guarantee that a workload can operate at the higher
*       frequencies permitted by this setting. Significantly more heat will be
*       generated at these high frequencies/voltages which will necessitate a
*       good cooling solution.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuVoltageOffsetSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double ocVoltageOffset                          ///< [in] The Turbo Overclocking Frequency Desired in mV.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuVoltageOffsetSet_t pfnOverclockGpuVoltageOffsetSet = (ctl_pfnOverclockGpuVoltageOffsetSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuVoltageOffsetSet");
        if (pfnOverclockGpuVoltageOffsetSet)
        {
            result = pfnOverclockGpuVoltageOffsetSet(hDeviceHandle, ocVoltageOffset);
        }
    }

    return result;
}


/**
* @brief Gets the Locked GPU Voltage for Overclocking in mV.
* 
* @details
*     - The purpose of this function is to determine if the current values of
*       the frequency/voltage lock.
*     - If the lock is not currently active, will return 0 for frequency and
*       voltage.
*     - Note that the operating frequency/voltage may be lower than these
*       settings if power/thermal limits are exceeded.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pVfPair`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuLockGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    ctl_oc_vf_pair_t* pVfPair                       ///< [out] The current locked voltage and frequency.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuLockGet_t pfnOverclockGpuLockGet = (ctl_pfnOverclockGpuLockGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuLockGet");
        if (pfnOverclockGpuLockGet)
        {
            result = pfnOverclockGpuLockGet(hDeviceHandle, pVfPair);
        }
    }

    return result;
}


/**
* @brief Locks the GPU voltage for Overclocking in mV.
* 
* @details
*     - The purpose of this function is to provide an interface for scanners
*       to lock the frequency and voltage to fixed values.
*     - The frequency is expressed in units of MHz with a resolution of 1MHz.
*     - The voltage is expressed in units of ±millivolts with values
*       permitted down to a resolution of 1 millivolt.
*     - The overclock waiver must be set since fixing the voltage at a high
*       value puts unnecessary stress on the part.
*     - The actual frequency may reduce depending on power/thermal
*       limitations.
*     - Requesting a frequency and/or voltage of 0 will return the hardware to
*       dynamic frequency/voltage management with any previous frequency
*       offset or voltage offset settings reapplied.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuLockSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    ctl_oc_vf_pair_t vFPair                         ///< [in] The current locked voltage and frequency.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuLockSet_t pfnOverclockGpuLockSet = (ctl_pfnOverclockGpuLockSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuLockSet");
        if (pfnOverclockGpuLockSet)
        {
            result = pfnOverclockGpuLockSet(hDeviceHandle, vFPair);
        }
    }

    return result;
}


/**
* @brief Get the current Vram Frequency Offset in GT/s.
* 
* @details
*     - The purpose of this function is to return the current VRAM frequency
*       offset in units of GT/s.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcFrequencyOffset`
*/
ctl_result_t CTL_APICALL
ctlOverclockVramFrequencyOffsetGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pOcFrequencyOffset                      ///< [in,out] The current Memory Frequency in GT/s.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockVramFrequencyOffsetGet_t pfnOverclockVramFrequencyOffsetGet = (ctl_pfnOverclockVramFrequencyOffsetGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockVramFrequencyOffsetGet");
        if (pfnOverclockVramFrequencyOffsetGet)
        {
            result = pfnOverclockVramFrequencyOffsetGet(hDeviceHandle, pOcFrequencyOffset);
        }
    }

    return result;
}


/**
* @brief Set the desired Vram frquency Offset in GT/s
* 
* @details
*     - The purpose of this function is to increase/decrease the frequency of
*       VRAM.
*     - The frequency offset is expressed in units of GT/s with a minimum step
*       size given by ::ctlOverclockGetProperties.
*     - The actual operating frequency for each workload is not guaranteed to
*       change exactly by the specified offset.
*     - The waiver must be set using clibOverclockWaiverSet() before this
*       function can be called.
*     - This setting is not persistent through system reboots or driver
*       resets/hangs. It is up to the overclock application to reapply the
*       settings in those cases.
*     - This setting can cause system/device instability. It is up to the
*       overclock application to detect if the system has rebooted
*       unexpectedly or the device was restarted. When this occurs, the
*       application should not reapply the overclock settings automatically
*       but instead return to previously known good settings or notify the
*       user that the settings are not being applied.
*     - If the memory controller doesn't support changes to frequency on the
*       fly, one of the following return codes will be given:
*     - ::CTL_RESULT_ERROR_RESET_DEVICE_REQUIRED: The requested memory
*       overclock will be applied when the device is reset or the system is
*       rebooted. In this case, the overclock software should check if the
*       overclock request was applied after the reset/reboot. If it was and
*       when the overclock application shuts down gracefully and if the
*       overclock application wants the setting to be persistent, the
*       application should request the same overclock settings again so that
*       they will be applied on the next reset/reboot. If this is not done,
*       then every time the device is reset and overclock is requested, the
*       device needs to be reset a second time.
*     - ::CTL_RESULT_ERROR_FULL_REBOOT_REQUIRED: The requested memory
*       overclock will be applied when the system is rebooted. In this case,
*       the overclock software should check if the overclock request was
*       applied after the reboot. If it was and when the overclock application
*       shuts down gracefully and if the overclock application wants the
*       setting to be persistent, the application should request the same
*       overclock settings again so that they will be applied on the next
*       reset/reboot. If this is not done and the overclock setting is
*       requested after the reboot has occurred, a second reboot will be
*       required.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockVramFrequencyOffsetSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double ocFrequencyOffset                        ///< [in] The desired Memory Frequency in GT/s.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockVramFrequencyOffsetSet_t pfnOverclockVramFrequencyOffsetSet = (ctl_pfnOverclockVramFrequencyOffsetSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockVramFrequencyOffsetSet");
        if (pfnOverclockVramFrequencyOffsetSet)
        {
            result = pfnOverclockVramFrequencyOffsetSet(hDeviceHandle, ocFrequencyOffset);
        }
    }

    return result;
}


/**
* @brief Get the Overclock Vram Voltage Offset in mV.
* 
* @details
*     - The purpose of this function is to increase/decrease the voltage of
*       VRAM.
*     - The voltage offset is expressed in units of millivolts with a minimum
*       step size given by ::ctlOverclockGetProperties.
*     - The waiver must be set using ::ctlOverclockWaiverSet before this
*       function can be called.
*     - This setting is not persistent through system reboots or driver
*       resets/hangs. It is up to the overclock application to reapply the
*       settings in those cases.
*     - This setting can cause system/device instability. It is up to the
*       overclock application to detect if the system has rebooted
*       unexpectedly or the device was restarted. When this occurs, the
*       application should not reapply the overclock settings automatically
*       but instead return to previously known good settings or notify the
*       user that the settings are not being applied.
*     - If the memory controller doesn't support changes to voltage on the
*       fly, one of the following return codes will be given:
*     - ::CTL_RESULT_ERROR_RESET_DEVICE_REQUIRED: The requested memory
*       overclock will be applied when the device is reset or the system is
*       rebooted. In this case, the overclock software should check if the
*       overclock request was applied after the reset/reboot. If it was and
*       when the overclock application shuts down gracefully and if the
*       overclock application wants the setting to be persistent, the
*       application should request the same overclock settings again so that
*       they will be applied on the next reset/reboot. If this is not done,
*       then every time the device is reset and overclock is requested, the
*       device needs to be reset a second time.
*     - ::CTL_RESULT_ERROR_FULL_REBOOT_REQUIRED: The requested memory
*       overclock will be applied when the system is rebooted. In this case,
*       the overclock software should check if the overclock request was
*       applied after the reboot. If it was and when the overclock application
*       shuts down gracefully and if the overclock application wants the
*       setting to be persistent, the application should request the same
*       overclock settings again so that they will be applied on the next
*       reset/reboot. If this is not done and the overclock setting is
*       requested after the reboot has occurred, a second reboot will be
*       required.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pVoltage`
*/
ctl_result_t CTL_APICALL
ctlOverclockVramVoltageOffsetGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pVoltage                                ///< [out] The current locked voltage in mV.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockVramVoltageOffsetGet_t pfnOverclockVramVoltageOffsetGet = (ctl_pfnOverclockVramVoltageOffsetGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockVramVoltageOffsetGet");
        if (pfnOverclockVramVoltageOffsetGet)
        {
            result = pfnOverclockVramVoltageOffsetGet(hDeviceHandle, pVoltage);
        }
    }

    return result;
}


/**
* @brief Set the Overclock Vram Voltage Offset in mV.
* 
* @details
*     - The purpose of this function is to set the maximum sustained power
*       limit. If the average GPU power averaged over a few seconds exceeds
*       this value, the frequency of the GPU will be throttled.
*     - Set a value of 0 to disable this power limit. In this case, the GPU
*       frequency will not throttle due to average power but may hit other
*       limits.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockVramVoltageOffsetSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double voltage                                  ///< [in] The voltage to be locked in mV.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockVramVoltageOffsetSet_t pfnOverclockVramVoltageOffsetSet = (ctl_pfnOverclockVramVoltageOffsetSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockVramVoltageOffsetSet");
        if (pfnOverclockVramVoltageOffsetSet)
        {
            result = pfnOverclockVramVoltageOffsetSet(hDeviceHandle, voltage);
        }
    }

    return result;
}


/**
* @brief Get the sustained power limit in mW.
* 
* @details
*     - The purpose of this function is to read the current sustained power
*       limit.
*     - A value of 0 means that the limit is disabled - the GPU frequency can
*       run as high as possible until other limits are hit.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSustainedPowerLimit`
*/
ctl_result_t CTL_APICALL
ctlOverclockPowerLimitGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pSustainedPowerLimit                    ///< [in,out] The current sustained power limit in mW.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockPowerLimitGet_t pfnOverclockPowerLimitGet = (ctl_pfnOverclockPowerLimitGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockPowerLimitGet");
        if (pfnOverclockPowerLimitGet)
        {
            result = pfnOverclockPowerLimitGet(hDeviceHandle, pSustainedPowerLimit);
        }
    }

    return result;
}


/**
* @brief Set the sustained power limit in mW.
* 
* @details
*     - The purpose of this function is to set the maximum sustained power
*       limit. If the average GPU power averaged over a few seconds exceeds
*       this value, the frequency of the GPU will be throttled.
*     - Set a value of 0 to disable this power limit. In this case, the GPU
*       frequency will not throttle due to average power but may hit other
*       limits.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockPowerLimitSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double sustainedPowerLimit                      ///< [in] The desired sustained power limit in mW.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockPowerLimitSet_t pfnOverclockPowerLimitSet = (ctl_pfnOverclockPowerLimitSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockPowerLimitSet");
        if (pfnOverclockPowerLimitSet)
        {
            result = pfnOverclockPowerLimitSet(hDeviceHandle, sustainedPowerLimit);
        }
    }

    return result;
}


/**
* @brief Get the current temperature limit in Celsius.
* 
* @details
*     - The purpose of this function is to read the current thermal limit.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pTemperatureLimit`
*/
ctl_result_t CTL_APICALL
ctlOverclockTemperatureLimitGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pTemperatureLimit                       ///< [in,out] The current temperature limit in Celsius.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockTemperatureLimitGet_t pfnOverclockTemperatureLimitGet = (ctl_pfnOverclockTemperatureLimitGet_t)GetProcAddress(hinstLibPtr, "ctlOverclockTemperatureLimitGet");
        if (pfnOverclockTemperatureLimitGet)
        {
            result = pfnOverclockTemperatureLimitGet(hDeviceHandle, pTemperatureLimit);
        }
    }

    return result;
}


/**
* @brief Set the temperature limit in Celsius.
* 
* @details
*     - The purpose of this function is to change the maximum thermal limit.
*       When the GPU temperature exceeds this value, the GPU frequency will be
*       throttled.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockTemperatureLimitSet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double temperatureLimit                         ///< [in] The desired temperature limit in Celsius.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockTemperatureLimitSet_t pfnOverclockTemperatureLimitSet = (ctl_pfnOverclockTemperatureLimitSet_t)GetProcAddress(hinstLibPtr, "ctlOverclockTemperatureLimitSet");
        if (pfnOverclockTemperatureLimitSet)
        {
            result = pfnOverclockTemperatureLimitSet(hDeviceHandle, temperatureLimit);
        }
    }

    return result;
}


/**
* @brief Get Power Telemetry.
* 
* @details
*     - Limited rate of 50 ms, any call under 50 ms will return the same
*       information.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pTelemetryInfo`
*/
ctl_result_t CTL_APICALL
ctlPowerTelemetryGet(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    ctl_power_telemetry_t* pTelemetryInfo           ///< [out] The overclocking properties for the specified domain.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPowerTelemetryGet_t pfnPowerTelemetryGet = (ctl_pfnPowerTelemetryGet_t)GetProcAddress(hinstLibPtr, "ctlPowerTelemetryGet");
        if (pfnPowerTelemetryGet)
        {
            result = pfnPowerTelemetryGet(hDeviceHandle, pTelemetryInfo);
        }
    }

    return result;
}


/**
* @brief Reset all Overclock Settings to stock
* 
* @details
*     - Reset all Overclock setting to default using single API call
*     - This request resets any changes made to GpuFrequencyOffset,
*       GpuVoltageOffset, PowerLimit, TemperatureLimit, GpuLock
*     - This Doesn't reset any Fan Curve Changes. It can be reset using
*       ctlFanSetDefaultMode
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockResetToDefault(
    ctl_device_adapter_handle_t hDeviceHandle       ///< [in][release] Handle to display adapter
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockResetToDefault_t pfnOverclockResetToDefault = (ctl_pfnOverclockResetToDefault_t)GetProcAddress(hinstLibPtr, "ctlOverclockResetToDefault");
        if (pfnOverclockResetToDefault)
        {
            result = pfnOverclockResetToDefault(hDeviceHandle);
        }
    }

    return result;
}


/**
* @brief Get the Current Overclock GPU Frequency Offset
* 
* @details
*     - Determine the current frequency offset in effect (refer to
*       ::ctlOverclockGpuFrequencyOffsetSetV2() for details).
*     - The unit of the value returned is given in
*       ::ctl_oc_properties_t::gpuFrequencyOffset::units returned from
*       ::ctlOverclockGetProperties()
*     - The unit of the value returned can be different for different
*       generation of graphics product
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcFrequencyOffset`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuFrequencyOffsetGetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pOcFrequencyOffset                      ///< [in,out] Current GPU Overclock Frequency Offset in units given in
                                                    ///< ::ctl_oc_properties_t::gpuFrequencyOffset::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuFrequencyOffsetGetV2_t pfnOverclockGpuFrequencyOffsetGetV2 = (ctl_pfnOverclockGpuFrequencyOffsetGetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuFrequencyOffsetGetV2");
        if (pfnOverclockGpuFrequencyOffsetGetV2)
        {
            result = pfnOverclockGpuFrequencyOffsetGetV2(hDeviceHandle, pOcFrequencyOffset);
        }
    }

    return result;
}


/**
* @brief Set the Overclock Frequency Offset for the GPU
* 
* @details
*     - The purpose of this function is to increase/decrease the frequency
*       offset at which typical workloads will run within the same thermal
*       budget.
*     - The frequency offset is expressed in units given in
*       ::ctl_oc_properties_t::gpuFrequencyOffset::units returned from
*       ::ctlOverclockGetProperties()
*     - The actual operating frequency for each workload is not guaranteed to
*       change exactly by the specified offset.
*     - For positive frequency offsets, the factory maximum frequency may
*       increase by up to the specified amount.
*     - Specifying large values for the frequency offset can lead to
*       instability. It is recommended that changes are made in small
*       increments and stability/performance measured running intense GPU
*       workloads before increasing further.
*     - This setting is not persistent through system reboots or driver
*       resets/hangs. It is up to the overclock application to reapply the
*       settings in those cases.
*     - This setting can cause system/device instability. It is up to the
*       overclock application to detect if the system has rebooted
*       unexpectedly or the device was restarted. When this occurs, the
*       application should not reapply the overclock settings automatically
*       but instead return to previously known good settings or notify the
*       user that the settings are not being applied.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuFrequencyOffsetSetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double ocFrequencyOffset                        ///< [in] The GPU Overclocking Frequency Offset Desired in units given in
                                                    ///< ::ctl_oc_properties_t::gpuFrequencyOffset::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuFrequencyOffsetSetV2_t pfnOverclockGpuFrequencyOffsetSetV2 = (ctl_pfnOverclockGpuFrequencyOffsetSetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuFrequencyOffsetSetV2");
        if (pfnOverclockGpuFrequencyOffsetSetV2)
        {
            result = pfnOverclockGpuFrequencyOffsetSetV2(hDeviceHandle, ocFrequencyOffset);
        }
    }

    return result;
}


/**
* @brief Get the Current Overclock Voltage Offset for the GPU
* 
* @details
*     - Determine the current maximum voltage offset in effect on the hardware
*       (refer to ::ctlOverclockGpuMaxVoltageOffsetSetV2 for details).
*     - The unit of the value returned is given in
*       ::ctl_oc_properties_t::gpuVoltageOffset::units returned from
*       ::ctlOverclockGetProperties()
*     - The unit of the value returned can be different for different
*       generation of graphics product
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcMaxVoltageOffset`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuMaxVoltageOffsetGetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pOcMaxVoltageOffset                     ///< [in,out] Current Overclock GPU Voltage Offset in Units given in
                                                    ///< ::ctl_oc_properties_t::gpuVoltageOffset::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuMaxVoltageOffsetGetV2_t pfnOverclockGpuMaxVoltageOffsetGetV2 = (ctl_pfnOverclockGpuMaxVoltageOffsetGetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuMaxVoltageOffsetGetV2");
        if (pfnOverclockGpuMaxVoltageOffsetGetV2)
        {
            result = pfnOverclockGpuMaxVoltageOffsetGetV2(hDeviceHandle, pOcMaxVoltageOffset);
        }
    }

    return result;
}


/**
* @brief Set the Overclock Voltage Offset for the GPU
* 
* @details
*     - The purpose of this function is to attempt to run the GPU up to higher
*       voltages beyond the part warrantee limits. This can permit running at
*       even higher frequencies than can be obtained using the frequency
*       offset setting, but at the risk of reducing the lifetime of the part.
*     - The voltage offset is expressed in units given in
*       ::ctl_oc_properties_t::gpuVoltageOffset::units returned from
*       ::ctlOverclockGetProperties()
*     - The overclock waiver must be set before calling this function
*       otherwise error will be returned.
*     - There is no guarantee that a workload can operate at the higher
*       frequencies permitted by this setting. Significantly more heat will be
*       generated at these high frequencies/voltages which will necessitate a
*       good cooling solution.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockGpuMaxVoltageOffsetSetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double ocMaxVoltageOffset                       ///< [in] The Overclocking Maximum Voltage Desired in units given in
                                                    ///< ::ctl_oc_properties_t::gpuVoltageOffset::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockGpuMaxVoltageOffsetSetV2_t pfnOverclockGpuMaxVoltageOffsetSetV2 = (ctl_pfnOverclockGpuMaxVoltageOffsetSetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockGpuMaxVoltageOffsetSetV2");
        if (pfnOverclockGpuMaxVoltageOffsetSetV2)
        {
            result = pfnOverclockGpuMaxVoltageOffsetSetV2(hDeviceHandle, ocMaxVoltageOffset);
        }
    }

    return result;
}


/**
* @brief Get the current Overclock Vram Memory Speed
* 
* @details
*     - The purpose of this function is to return the current VRAM Memory
*       Speed
*     - The unit of the value returned is given in
*       ::ctl_oc_properties_t::vramMemSpeedLimit::units returned from
*       ::ctlOverclockGetProperties()
*     - The unit of the value returned can be different for different
*       generation of graphics product
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pOcVramMemSpeedLimit`
*/
ctl_result_t CTL_APICALL
ctlOverclockVramMemSpeedLimitGetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pOcVramMemSpeedLimit                    ///< [in,out] The current VRAM Memory Speed in units given in
                                                    ///< ::ctl_oc_properties_t::vramMemSpeedLimit::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockVramMemSpeedLimitGetV2_t pfnOverclockVramMemSpeedLimitGetV2 = (ctl_pfnOverclockVramMemSpeedLimitGetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockVramMemSpeedLimitGetV2");
        if (pfnOverclockVramMemSpeedLimitGetV2)
        {
            result = pfnOverclockVramMemSpeedLimitGetV2(hDeviceHandle, pOcVramMemSpeedLimit);
        }
    }

    return result;
}


/**
* @brief Set the desired Overclock Vram Memory Speed
* 
* @details
*     - The purpose of this function is to increase/decrease the Speed of
*       VRAM.
*     - The Memory Speed is expressed in units given in
*       ::ctl_oc_properties_t::vramMemSpeedLimit::units returned from
*       ::ctlOverclockGetProperties() with a minimum step size given by
*       ::ctlOverclockGetProperties().
*     - The actual Memory Speed for each workload is not guaranteed to change
*       exactly by the specified offset.
*     - This setting is not persistent through system reboots or driver
*       resets/hangs. It is up to the overclock application to reapply the
*       settings in those cases.
*     - This setting can cause system/device instability. It is up to the
*       overclock application to detect if the system has rebooted
*       unexpectedly or the device was restarted. When this occurs, the
*       application should not reapply the overclock settings automatically
*       but instead return to previously known good settings or notify the
*       user that the settings are not being applied.
*     - If the memory controller doesn't support changes to memory speed on
*       the fly, one of the following return codes will be given:
*     - CTL_RESULT_ERROR_RESET_DEVICE_REQUIRED: The requested memory overclock
*       will be applied when the device is reset or the system is rebooted. In
*       this case, the overclock software should check if the overclock
*       request was applied after the reset/reboot. If it was and when the
*       overclock application shuts down gracefully and if the overclock
*       application wants the setting to be persistent, the application should
*       request the same overclock settings again so that they will be applied
*       on the next reset/reboot. If this is not done, then every time the
*       device is reset and overclock is requested, the device needs to be
*       reset a second time.
*     - CTL_RESULT_ERROR_FULL_REBOOT_REQUIRED: The requested memory overclock
*       will be applied when the system is rebooted. In this case, the
*       overclock software should check if the overclock request was applied
*       after the reboot. If it was and when the overclock application shuts
*       down gracefully and if the overclock application wants the setting to
*       be persistent, the application should request the same overclock
*       settings again so that they will be applied on the next reset/reboot.
*       If this is not done and the overclock setting is requested after the
*       reboot has occurred, a second reboot will be required.
*     - CTL_RESULT_ERROR_UNSUPPORTED_FEATURE: The Memory Speed Get / Set
*       Feature is currently not available or Unsupported in current platform
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockVramMemSpeedLimitSetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double ocVramMemSpeedLimit                      ///< [in] The desired Memory Speed in units given in
                                                    ///< ::ctl_oc_properties_t::vramMemSpeedLimit::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockVramMemSpeedLimitSetV2_t pfnOverclockVramMemSpeedLimitSetV2 = (ctl_pfnOverclockVramMemSpeedLimitSetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockVramMemSpeedLimitSetV2");
        if (pfnOverclockVramMemSpeedLimitSetV2)
        {
            result = pfnOverclockVramMemSpeedLimitSetV2(hDeviceHandle, ocVramMemSpeedLimit);
        }
    }

    return result;
}


/**
* @brief Get the Current Sustained power limit
* 
* @details
*     - The purpose of this function is to read the current sustained power
*       limit.
*     - The unit of the value returned is given in
*       ::ctl_oc_properties_t::powerLimit::units returned from
*       ::ctlOverclockGetProperties()
*     - The unit of the value returned can be different for different
*       generation of graphics product
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pSustainedPowerLimit`
*/
ctl_result_t CTL_APICALL
ctlOverclockPowerLimitGetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pSustainedPowerLimit                    ///< [in,out] The current Sustained Power limit in Units given in
                                                    ///< ::ctl_oc_properties_t::powerLimit::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockPowerLimitGetV2_t pfnOverclockPowerLimitGetV2 = (ctl_pfnOverclockPowerLimitGetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockPowerLimitGetV2");
        if (pfnOverclockPowerLimitGetV2)
        {
            result = pfnOverclockPowerLimitGetV2(hDeviceHandle, pSustainedPowerLimit);
        }
    }

    return result;
}


/**
* @brief Set the Sustained power limit
* 
* @details
*     - The purpose of this function is to set the maximum sustained power
*       limit. If the average GPU power averaged over a few seconds exceeds
*       this value, the frequency of the GPU will be throttled.
*     - Set a value of 0 to disable this power limit. In this case, the GPU
*       frequency will not throttle due to average power but may hit other
*       limits.
*     - The unit of the PowerLimit to be set is given in
*       ::ctl_oc_properties_t::powerLimit::units returned from
*       ::ctlOverclockGetProperties()
*     - The unit of the value returned can be different for different
*       generation of graphics product
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockPowerLimitSetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double sustainedPowerLimit                      ///< [in] The desired sustained power limit in Units given in
                                                    ///< ::ctl_oc_properties_t::powerLimit::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockPowerLimitSetV2_t pfnOverclockPowerLimitSetV2 = (ctl_pfnOverclockPowerLimitSetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockPowerLimitSetV2");
        if (pfnOverclockPowerLimitSetV2)
        {
            result = pfnOverclockPowerLimitSetV2(hDeviceHandle, sustainedPowerLimit);
        }
    }

    return result;
}


/**
* @brief Get the current temperature limit
* 
* @details
*     - The purpose of this function is to read the current thermal limit used
*       for Overclocking
*     - The unit of the value returned is given in
*       ::ctl_oc_properties_t::temperatureLimit::units returned from
*       ::ctlOverclockGetProperties()
*     - The unit of the value returned can be different for different
*       generation of graphics product
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pTemperatureLimit`
*/
ctl_result_t CTL_APICALL
ctlOverclockTemperatureLimitGetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double* pTemperatureLimit                       ///< [in,out] The current temperature limit in Units given in
                                                    ///< ::ctl_oc_properties_t::temperatureLimit::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockTemperatureLimitGetV2_t pfnOverclockTemperatureLimitGetV2 = (ctl_pfnOverclockTemperatureLimitGetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockTemperatureLimitGetV2");
        if (pfnOverclockTemperatureLimitGetV2)
        {
            result = pfnOverclockTemperatureLimitGetV2(hDeviceHandle, pTemperatureLimit);
        }
    }

    return result;
}


/**
* @brief Set the temperature limit
* 
* @details
*     - The purpose of this function is to change the maximum thermal limit.
*       When the GPU temperature exceeds this value, the GPU frequency will be
*       throttled.
*     - The unit of the value to be set is given in
*       ::ctl_oc_properties_t::temperatureLimit::units returned from
*       ::ctlOverclockGetProperties()
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceHandle`
*/
ctl_result_t CTL_APICALL
ctlOverclockTemperatureLimitSetV2(
    ctl_device_adapter_handle_t hDeviceHandle,      ///< [in][release] Handle to display adapter
    double temperatureLimit                         ///< [in] The desired temperature limit in Units given in
                                                    ///< ::ctl_oc_properties_t::temperatureLimit::units returned from
                                                    ///< ::ctlOverclockGetProperties()
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockTemperatureLimitSetV2_t pfnOverclockTemperatureLimitSetV2 = (ctl_pfnOverclockTemperatureLimitSetV2_t)GetProcAddress(hinstLibPtr, "ctlOverclockTemperatureLimitSetV2");
        if (pfnOverclockTemperatureLimitSetV2)
        {
            result = pfnOverclockTemperatureLimitSetV2(hDeviceHandle, temperatureLimit);
        }
    }

    return result;
}


/**
* @brief Read VF Curve
* 
* @details
*     - Read the Voltage-Frequency Curve
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_ENUMERATION
*         + `::CTL_VF_CURVE_TYPE_LIVE < VFCurveType`
*         + `::CTL_VF_CURVE_DETAILS_ELABORATE < VFCurveDetail`
*     - CTL_RESULT_ERROR_UNKNOWN - "Unknown Error"
*/
ctl_result_t CTL_APICALL
ctlOverclockReadVFCurve(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] Handle to control device adapter
    ctl_vf_curve_type_t VFCurveType,                ///< [in] Type of Curve to read
    ctl_vf_curve_details_t VFCurveDetail,           ///< [in] Detail of Curve to read
    uint32_t * pNumPoints,                          ///< [in][out] Number of points in the custom VF curve. If the NumPoints is
                                                    ///< zero, then the api will update the value with total number of Points
                                                    ///< based on requested VFCurveType and VFCurveDetail. If the NumPoints is
                                                    ///< non-zero, then the api will read and update the VF points in
                                                    ///< pVFCurveTable buffer provided. If the NumPoints doesn't match what the
                                                    ///< api returned in the first call, it will return an error.
    ctl_voltage_frequency_point_t * pVFCurveTable   ///< [in][out] Pointer to array of VF points, to copy the VF curve being
                                                    ///< read
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockReadVFCurve_t pfnOverclockReadVFCurve = (ctl_pfnOverclockReadVFCurve_t)GetProcAddress(hinstLibPtr, "ctlOverclockReadVFCurve");
        if (pfnOverclockReadVFCurve)
        {
            result = pfnOverclockReadVFCurve(hDeviceAdapter, VFCurveType, VFCurveDetail, pNumPoints, pVFCurveTable);
        }
    }

    return result;
}


/**
* @brief Write Custom VF curve
* 
* @details
*     - Modify the Voltage-Frequency Curve used by GPU
*     - Valid Voltage-Frequency Curve shall have Voltage and Frequency Points
*       in increasing order
*     - Recommended to create Custom V-F Curve from reading Current V-F Curve
*       using ::ctlOverclockReadVFCurve (Read-Modify-Write)
*     - If Custom V-F curve write request is Successful, the Applied VF Curve
*       might be slightly different than what is originally requested,
*       recommended to update the UI by reading the V-F curve again using
*       ctlOverclockReadVFCurve (with ctl_vf_curve_type_t::LIVE as input)
*     - The overclock waiver must be set before calling this function
*       otherwise error will be returned.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDeviceAdapter`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCustomVFCurveTable`
*     - CTL_RESULT_ERROR_UNKNOWN - "Unknown Error"
*/
ctl_result_t CTL_APICALL
ctlOverclockWriteCustomVFCurve(
    ctl_device_adapter_handle_t hDeviceAdapter,     ///< [in][release] Handle to control device adapter
    uint32_t NumPoints,                             ///< [in] Number of points in the custom VF curve
    ctl_voltage_frequency_point_t* pCustomVFCurveTable  ///< [in] Pointer to an array of VF Points containing 'NumPoints' Custom VF
                                                    ///< points
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnOverclockWriteCustomVFCurve_t pfnOverclockWriteCustomVFCurve = (ctl_pfnOverclockWriteCustomVFCurve_t)GetProcAddress(hinstLibPtr, "ctlOverclockWriteCustomVFCurve");
        if (pfnOverclockWriteCustomVFCurve)
        {
            result = pfnOverclockWriteCustomVFCurve(hDeviceAdapter, NumPoints, pCustomVFCurveTable);
        }
    }

    return result;
}


/**
* @brief Get PCI properties - address, max speed
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlPciGetProperties(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_pci_properties_t* pProperties               ///< [in,out] Will contain the PCI properties.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPciGetProperties_t pfnPciGetProperties = (ctl_pfnPciGetProperties_t)GetProcAddress(hinstLibPtr, "ctlPciGetProperties");
        if (pfnPciGetProperties)
        {
            result = pfnPciGetProperties(hDAhandle, pProperties);
        }
    }

    return result;
}


/**
* @brief Get current PCI state - current speed
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pState`
*/
ctl_result_t CTL_APICALL
ctlPciGetState(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    ctl_pci_state_t* pState                         ///< [in,out] Will contain the PCI properties.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPciGetState_t pfnPciGetState = (ctl_pfnPciGetState_t)GetProcAddress(hinstLibPtr, "ctlPciGetState");
        if (pfnPciGetState)
        {
            result = pfnPciGetState(hDAhandle, pState);
        }
    }

    return result;
}


/**
* @brief Get handle of power domains
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumPowerDomains(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_pwr_handle_t* phPower                       ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< if count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumPowerDomains_t pfnEnumPowerDomains = (ctl_pfnEnumPowerDomains_t)GetProcAddress(hinstLibPtr, "ctlEnumPowerDomains");
        if (pfnEnumPowerDomains)
        {
            result = pfnEnumPowerDomains(hDAhandle, pCount, phPower);
        }
    }

    return result;
}


/**
* @brief Get properties related to a power domain
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPower`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlPowerGetProperties(
    ctl_pwr_handle_t hPower,                        ///< [in] Handle for the component.
    ctl_power_properties_t* pProperties             ///< [in,out] Structure that will contain property data.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPowerGetProperties_t pfnPowerGetProperties = (ctl_pfnPowerGetProperties_t)GetProcAddress(hinstLibPtr, "ctlPowerGetProperties");
        if (pfnPowerGetProperties)
        {
            result = pfnPowerGetProperties(hPower, pProperties);
        }
    }

    return result;
}


/**
* @brief Get energy counter
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPower`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pEnergy`
*/
ctl_result_t CTL_APICALL
ctlPowerGetEnergyCounter(
    ctl_pwr_handle_t hPower,                        ///< [in] Handle for the component.
    ctl_power_energy_counter_t* pEnergy             ///< [in,out] Will contain the latest snapshot of the energy counter and
                                                    ///< timestamp when the last counter value was measured.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPowerGetEnergyCounter_t pfnPowerGetEnergyCounter = (ctl_pfnPowerGetEnergyCounter_t)GetProcAddress(hinstLibPtr, "ctlPowerGetEnergyCounter");
        if (pfnPowerGetEnergyCounter)
        {
            result = pfnPowerGetEnergyCounter(hPower, pEnergy);
        }
    }

    return result;
}


/**
* @brief Get power limits
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPower`
*/
ctl_result_t CTL_APICALL
ctlPowerGetLimits(
    ctl_pwr_handle_t hPower,                        ///< [in] Handle for the component.
    ctl_power_limits_t* pPowerLimits                ///< [in,out][optional] Structure that will contain the power limits.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPowerGetLimits_t pfnPowerGetLimits = (ctl_pfnPowerGetLimits_t)GetProcAddress(hinstLibPtr, "ctlPowerGetLimits");
        if (pfnPowerGetLimits)
        {
            result = pfnPowerGetLimits(hPower, pPowerLimits);
        }
    }

    return result;
}


/**
* @brief Set power limits
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hPower`
*     - ::CTL_RESULT_ERROR_INSUFFICIENT_PERMISSIONS
*         + User does not have permissions to make these modifications.
*     - ::CTL_RESULT_ERROR_NOT_AVAILABLE
*         + The device is in use, meaning that the GPU is under Over clocking, applying power limits under overclocking is not supported.
*/
ctl_result_t CTL_APICALL
ctlPowerSetLimits(
    ctl_pwr_handle_t hPower,                        ///< [in] Handle for the component.
    const ctl_power_limits_t* pPowerLimits          ///< [in][optional] Structure that will contain the power limits.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnPowerSetLimits_t pfnPowerSetLimits = (ctl_pfnPowerSetLimits_t)GetProcAddress(hinstLibPtr, "ctlPowerSetLimits");
        if (pfnPowerSetLimits)
        {
            result = pfnPowerSetLimits(hPower, pPowerLimits);
        }
    }

    return result;
}


/**
* @brief Get handle of temperature sensors
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hDAhandle`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pCount`
*/
ctl_result_t CTL_APICALL
ctlEnumTemperatureSensors(
    ctl_device_adapter_handle_t hDAhandle,          ///< [in][release] Handle to display adapter
    uint32_t* pCount,                               ///< [in,out] pointer to the number of components of this type.
                                                    ///< if count is zero, then the driver shall update the value with the
                                                    ///< total number of components of this type that are available.
                                                    ///< if count is greater than the number of components of this type that
                                                    ///< are available, then the driver shall update the value with the correct
                                                    ///< number of components.
    ctl_temp_handle_t* phTemperature                ///< [in,out][optional][range(0, *pCount)] array of handle of components of
                                                    ///< this type.
                                                    ///< if count is less than the number of components of this type that are
                                                    ///< available, then the driver shall only retrieve that number of
                                                    ///< component handles.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnEnumTemperatureSensors_t pfnEnumTemperatureSensors = (ctl_pfnEnumTemperatureSensors_t)GetProcAddress(hinstLibPtr, "ctlEnumTemperatureSensors");
        if (pfnEnumTemperatureSensors)
        {
            result = pfnEnumTemperatureSensors(hDAhandle, pCount, phTemperature);
        }
    }

    return result;
}


/**
* @brief Get temperature sensor properties
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hTemperature`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pProperties`
*/
ctl_result_t CTL_APICALL
ctlTemperatureGetProperties(
    ctl_temp_handle_t hTemperature,                 ///< [in] Handle for the component.
    ctl_temp_properties_t* pProperties              ///< [in,out] Will contain the temperature sensor properties.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnTemperatureGetProperties_t pfnTemperatureGetProperties = (ctl_pfnTemperatureGetProperties_t)GetProcAddress(hinstLibPtr, "ctlTemperatureGetProperties");
        if (pfnTemperatureGetProperties)
        {
            result = pfnTemperatureGetProperties(hTemperature, pProperties);
        }
    }

    return result;
}


/**
* @brief Get the temperature from a specified sensor
* 
* @details
*     - The application may call this function from simultaneous threads.
*     - The implementation of this function should be lock-free.
* 
* @returns
*     - CTL_RESULT_SUCCESS
*     - CTL_RESULT_ERROR_UNINITIALIZED
*     - CTL_RESULT_ERROR_DEVICE_LOST
*     - CTL_RESULT_ERROR_INVALID_NULL_HANDLE
*         + `nullptr == hTemperature`
*     - CTL_RESULT_ERROR_INVALID_NULL_POINTER
*         + `nullptr == pTemperature`
*/
ctl_result_t CTL_APICALL
ctlTemperatureGetState(
    ctl_temp_handle_t hTemperature,                 ///< [in] Handle for the component.
    double* pTemperature                            ///< [in,out] Will contain the temperature read from the specified sensor
                                                    ///< in degrees Celsius.
    )
{
    ctl_result_t result = CTL_RESULT_ERROR_NOT_INITIALIZED;
    

    HINSTANCE hinstLibPtr = GetLoaderHandle();

    if (NULL != hinstLibPtr)
    {
        ctl_pfnTemperatureGetState_t pfnTemperatureGetState = (ctl_pfnTemperatureGetState_t)GetProcAddress(hinstLibPtr, "ctlTemperatureGetState");
        if (pfnTemperatureGetState)
        {
            result = pfnTemperatureGetState(hTemperature, pTemperature);
        }
    }

    return result;
}


//
// End of wrapper function implementation
//
/////////////////////////////////////////////////////////////////////////////////
