#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <setupapi.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "usbhostfs_pc.h"
#include "device.h"

typedef PVOID WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

#define PIPE_TRANSFER_TIMEOUT   0x03

static GUID GUID_UsbHostFS = { 0x251fab1c, 0xc0d7, 0x4536, { 0x9e, 0x58, 0x8f, 0x7c, 0x6f, 0x64, 0x12, 0xe6 } };

typedef BOOL (__stdcall *fWinUsb_Initialize)(
    IN HANDLE DeviceHandle,
    OUT PWINUSB_INTERFACE_HANDLE InterfaceHandle
    );
    
typedef BOOL (__stdcall *fWinUsb_Free)(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle
    );

typedef BOOL (__stdcall *fWinUsb_SetPipePolicy)(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID,
    IN  ULONG PolicyType,
    IN  ULONG ValueLength,
    IN  PVOID Value
    );

typedef BOOL (__stdcall *fWinUsb_ReadPipe)(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID,
    IN  PUCHAR Buffer,
    IN  ULONG BufferLength,
    OUT PULONG LengthTransferred,
    IN  LPOVERLAPPED Overlapped
    );

typedef BOOL (__stdcall *fWinUsb_WritePipe)(
    IN  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    IN  UCHAR PipeID,
    IN  PUCHAR Buffer,
    IN  ULONG BufferLength,
    OUT PULONG LengthTransferred,
    IN  LPOVERLAPPED Overlapped    
    );

struct winusb_device
{
	HANDLE hFile;
	WINUSB_INTERFACE_HANDLE hDevice;
};

static HMODULE g_hWinUsb = NULL;
static fWinUsb_Initialize pfWinUsb_Initialize;
static fWinUsb_Free pfWinUsb_Free;
static fWinUsb_SetPipePolicy pfWinUsb_SetPipePolicy;
static fWinUsb_ReadPipe pfWinUsb_ReadPipe;
static fWinUsb_WritePipe pfWinUsb_WritePipe;

void device_init(void)
{
	g_hWinUsb = LoadLibrary("winusb");
	if(g_hWinUsb)
	{
		pfWinUsb_Initialize = (fWinUsb_Initialize)GetProcAddress(g_hWinUsb, "WinUsb_Initialize");
		pfWinUsb_Free = (fWinUsb_Free)GetProcAddress(g_hWinUsb, "WinUsb_Free");
		pfWinUsb_SetPipePolicy = (fWinUsb_SetPipePolicy)GetProcAddress(g_hWinUsb, "WinUsb_SetPipePolicy");
		pfWinUsb_ReadPipe = (fWinUsb_ReadPipe)GetProcAddress(g_hWinUsb, "WinUsb_ReadPipe");
		pfWinUsb_WritePipe = (fWinUsb_WritePipe)GetProcAddress(g_hWinUsb, "WinUsb_WritePipe");
	}
	else
	{
		fprintf(stderr, "Could not load winusb library\n");
		exit(1);
	}
}


BOOL GetDevicePath(LPGUID InterfaceGuid,
                   LPSTR DevicePath,
                   size_t BufLen)
{
  BOOL bResult = FALSE;
  HDEVINFO deviceInfo;
  SP_DEVICE_INTERFACE_DATA interfaceData;
  PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
  ULONG length;
  ULONG requiredLength=0;
  HRESULT hr;

// [1]
  deviceInfo = SetupDiGetClassDevs(InterfaceGuid,
                     NULL, NULL,
                     DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if(deviceInfo == INVALID_HANDLE_VALUE)
	{
		bResult = FALSE;
		goto endFunction;
	}

// [2]
  interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

  bResult = SetupDiEnumDeviceInterfaces(deviceInfo,
                                        NULL,
                                        InterfaceGuid,
                                        0,
                                        &interfaceData);
  if(!bResult)
  {
	  goto endFunction;
  }
//Error handling code omitted.

// [3]
  SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                  &interfaceData,
                                  NULL, 0,
                                  &requiredLength,
                                  NULL);

  detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
     LocalAlloc(LMEM_FIXED, requiredLength);

  if(NULL == detailData)
  {
    SetupDiDestroyDeviceInfoList(deviceInfo);
    return FALSE;
  }

  detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
  length = requiredLength;

  bResult = SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                           &interfaceData,
                                           detailData,
                                           length,
                                           &requiredLength,
                                           NULL);

  if(FALSE == bResult)
  {
    LocalFree(detailData);
    goto endFunction;
  }

// [4]
  hr = snprintf(DevicePath, BufLen, "%s", detailData->DevicePath);
  if(FAILED(hr))
  {
    SetupDiDestroyDeviceInfoList(deviceInfo);
    LocalFree(detailData);
	goto endFunction;
  }

  LocalFree(detailData);

endFunction:

  return bResult;
}

device_handle device_open(void)
{
	CHAR DevicePath[MAX_PATH];
	BOOL bResult = FALSE;
	struct winusb_device* dev = NULL;

	dev = (struct winusb_device*)malloc(sizeof(struct winusb_device));
	if(dev)
	{
		dev->hFile = INVALID_HANDLE_VALUE;
		dev->hDevice = NULL;

		if(GetDevicePath(&GUID_UsbHostFS, DevicePath, MAX_PATH))
		{
			V_PRINTF(2, "Device Path: %s\n", DevicePath);
			dev->hFile = CreateFile(DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
			if(dev->hFile != INVALID_HANDLE_VALUE)
			{
				bResult = pfWinUsb_Initialize(dev->hFile, &dev->hDevice);
				if(!bResult)
				{
					fprintf(stderr, "Error initialising WinUSB %d\n", GetLastError());
				}
			}
			else
			{
				V_PRINTF(2, "Error opening device %d\n", GetLastError());
			}
		}

		if(!bResult)
		{
			device_close((device_handle)dev);
			dev = NULL;
		}
	}

	return (device_handle)dev;
}

void device_close(device_handle device)
{
	struct winusb_device* dev = (struct winusb_device*)device;

	if(dev)
	{
		if(dev->hDevice)
		{
			pfWinUsb_Free(dev->hDevice);
			dev->hDevice = NULL;
		}

		if(dev->hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(dev->hFile);
			dev->hFile = INVALID_HANDLE_VALUE;
		}
		free(dev);
	}
}

int device_write(device_handle device, int ep, char* bytes, int size, int timeout)
{
	int iRet = -1;
	BOOL bResult;
	DWORD length;
	struct winusb_device* dev = (struct winusb_device*) device;

	if(dev && dev->hDevice)
	{
		bResult = pfWinUsb_SetPipePolicy(dev->hDevice, ep, PIPE_TRANSFER_TIMEOUT, sizeof(timeout), &timeout);
		if(bResult)
		{
			bResult = pfWinUsb_WritePipe(dev->hDevice, ep, (PUCHAR)bytes, size, &length, NULL);
			if(bResult)
			{
				iRet = length;
			}
			else if(GetLastError() == ERROR_SEM_TIMEOUT)
			{
				iRet = -ETIMEDOUT;
			}
			else
			{
				iRet = -1;
			}
		}
	}

	return iRet;
}

int device_read(device_handle device, int ep, char* bytes, int size, int timeout)
{
	int iRet = -1;
	BOOL bResult;
	DWORD length;
	struct winusb_device* dev = (struct winusb_device*) device;

	if(dev && dev->hDevice)
	{
		bResult = pfWinUsb_SetPipePolicy(dev->hDevice, ep, PIPE_TRANSFER_TIMEOUT, sizeof(timeout), &timeout);
		if(bResult)
		{
			bResult = pfWinUsb_ReadPipe(dev->hDevice, ep, (PUCHAR)bytes, size, &length, NULL);
			if(bResult)
			{
				iRet = length;
			}
			else if(GetLastError() == ERROR_SEM_TIMEOUT)
			{
				iRet = -ETIMEDOUT;
			}
			else
			{
				iRet = -1;
			}
		}
	}

	return iRet;
}
