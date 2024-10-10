// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Service.h"

#include <assert.h>
#include <strsafe.h>
#include <dbt.h>
#include <format>
#include "PMMainThread.h"
#include <iostream>

const GUID GUID_DEVINTERFACE_DISPLAY_ADAPTER = {
	0x5B45201D,
	0xF2F2,
	0x4F3B,
	{ 0x85, 0xBB, 0x30, 0xFF, 0x1F, 0x95, 0x35, 0x99 },
};

DWORD ServiceCtrlHandlerEx(DWORD control, DWORD dwEventType, LPVOID lpEventData,
	LPVOID lpContext)
{
	ConcreteService* service_in = (ConcreteService*)lpContext;
	switch (control) {
	case SERVICE_CONTROL_STOP:
		service_in->ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		service_in->SignalServiceStop();
		return NO_ERROR;
	case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
		return NO_ERROR;
	case SERVICE_CONTROL_INTERROGATE:
		return NO_ERROR;
	case SERVICE_CONTROL_DEVICEEVENT:
		if (dwEventType == DBT_DEVICEARRIVAL) {
			SetEvent(service_in->GetResetPowerTelemetryHandle());
		}
		return NO_ERROR;
	default:
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
}

Service::Service()
	:
	clientSessionEvent_{ CreateEventA(nullptr, TRUE, FALSE, nullptr) }
{}

void Service::SignalServiceStop(std::optional<int> errCode)
{
	errCode_ = errCode;
	SetEvent(GetServiceStopHandle());
}

std::optional<int> Service::GetErrorCode() const
{
	return errCode_;
}

HANDLE Service::GetClientSessionHandle()
{
	return clientSessionEvent_;
}

void Service::SignalClientSessionOpened()
{
	SetEvent(clientSessionEvent_);
}

Service::~Service()
{
	if (clientSessionEvent_) {
		CloseHandle(clientSessionEvent_);
	}
}

ConcreteService::ConcreteService(const TCHAR* serviceName) : mServiceName(serviceName)
{
	mEventLogHandle = RegisterEventSource(NULL, mServiceName.c_str());
}

void ConcreteService::ServiceMain()
{
	mServiceStatusHandle = RegisterServiceCtrlHandlerEx(
		mServiceName.c_str(), ServiceCtrlHandlerEx, this);

	// If unable to register the service control handle, have to
	// bail
	if (mServiceStatusHandle == nullptr) {
		return;
	}

	// Next setup to receive notfications to monitor if the display
	// adapters on the system are changed so we can update the telemetry
	// providers
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter{};
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_DISPLAY_ADAPTER;
	mDeviceNotifyHandle = RegisterDeviceNotification(
		mServiceStatusHandle, &NotificationFilter, DEVICE_NOTIFY_SERVICE_HANDLE);
	if (mDeviceNotifyHandle == nullptr) {
		return;
	}

	mServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	mServiceStatus.dwServiceSpecificExitCode = 0;

	ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);


	ServiceInit();

	return;
}

void ConcreteService::ServiceInit()
{
	// Create an event. The control handler function, ServiceCtrlHandler,
	// signals this event when it receives the stop control code from the
	// SCM.
	mServiceStopEventHandle = CreateEvent(NULL,   // default security attributes
		TRUE,   // manual reset event
		FALSE,  // not signaled
		NULL);  // no name

	if (mServiceStopEventHandle == nullptr) {
		ReportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	mResetPowerTelemetryEventHandle = CreateEvent(NULL,   // default security attributes
		FALSE,          // auto reset event
		FALSE,          // not signaled
		NULL);          // no name
	if (mServiceStopEventHandle == nullptr) {
		ReportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	// Start the main PresentMon thread
	std::thread pmMainThread(PresentMonMainThread, this);

	ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// Check if we should stop our service
	WaitForSingleObject(mServiceStopEventHandle, INFINITE);

	if (pmMainThread.joinable()) {
		pmMainThread.join();
	}

	ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);

	if (mEventLogHandle) {
		DeregisterEventSource(mEventLogHandle);
		mEventLogHandle = nullptr;
	}
	if (mServiceStopEventHandle) {
		CloseHandle(mServiceStopEventHandle);
		mServiceStopEventHandle = nullptr;
	}
	if (mDeviceNotifyHandle) {
		UnregisterDeviceNotification(mDeviceNotifyHandle);
		mDeviceNotifyHandle = nullptr;
	}

	return;
}

void ConcreteService::ReportServiceStatus(DWORD currentState, DWORD win32ExitCode,
	DWORD waitHint)
{
	mServiceStatus.dwCurrentState = currentState;
	mServiceStatus.dwWin32ExitCode = win32ExitCode;
	mServiceStatus.dwWaitHint = waitHint;

	if (currentState == SERVICE_START_PENDING) {
		// If we are in a PENDING state do not accept any controls.
		mServiceStatus.dwControlsAccepted = 0;
		// Increment the check point to indicate to the SCM progress
		// is being made
		mServiceStatus.dwCheckPoint = mCheckPoint++;
	}
	else if (currentState == SERVICE_RUNNING ||
		currentState == SERVICE_STOP_PENDING ||
		currentState == SERVICE_STOPPED) {
		// If we transitioned to a RUNNING, STOP_PENDING or STOPPED state
		// we accept a STOP control
		mServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		mServiceStatus.dwCheckPoint = 0;
	}
	else {
		// We only set START_PENDING, RUNNING, STOP_PENDING and STOPPED.
		// Please check this incoming currentState.
		assert(false);
	}

	SetServiceStatus(mServiceStatusHandle, &mServiceStatus);
}


// debug mock service
ConsoleDebugMockService::ConsoleDebugMockService()
{
	stopEvent_ = CreateEvent(
		nullptr,   // default security attributes
		TRUE,      // manual-reset event
		FALSE,     // initial state is nonsignaled
		nullptr    // object name
	);
	resetTelemetryEvent_ = CreateEvent(
		nullptr,   // default security attributes
		TRUE,      // manual-reset event
		FALSE,     // initial state is nonsignaled
		nullptr    // object name
	);
	SetConsoleCtrlHandler(&ConsoleHandler, TRUE);
}

ConsoleDebugMockService::~ConsoleDebugMockService()
{
	if (stopEvent_) {
		CloseHandle(stopEvent_);
	}
	if (resetTelemetryEvent_) {
		CloseHandle(resetTelemetryEvent_);
	}
}

BOOL WINAPI ConsoleDebugMockService::ConsoleHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
		Get().SignalServiceStop();
		return TRUE;
	}
	return FALSE;
}

ConsoleDebugMockService& ConsoleDebugMockService::Get()
{
	static ConsoleDebugMockService svc;
	return svc;
}

void ConsoleDebugMockService::Run()
{
	std::cout << "Running service as a console app..." << std::endl;

	// Start the main PresentMon thread, wait for it to exit
	std::jthread pmMainThread(PresentMonMainThread, this);
}

HANDLE ConsoleDebugMockService::GetServiceStopHandle()
{
	return stopEvent_;
}

HANDLE ConsoleDebugMockService::GetResetPowerTelemetryHandle()
{
	return resetTelemetryEvent_;
}
