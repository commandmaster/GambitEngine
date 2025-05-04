#pragma once

#include <Windows.h>
#include <string>
#include <stdexcept>
#include <iostream>

HANDLE getSerialHandle(uint8_t comPort)
{
    std::wstring portName = L"\\\\.\\COM" + std::to_wstring(comPort);

    HANDLE hSerial = CreateFile(
        portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) 
    {
        std::cerr << "Error opening COM" << (int)comPort << "\n";
        throw std::runtime_error("Failed to open serial port");
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) 
    {
        std::cerr << "Error getting state\n";
        CloseHandle(hSerial);
        throw std::runtime_error("Failed to get serial port state");
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) 
    {
        std::cerr << "Error setting state\n";
        CloseHandle(hSerial);
        throw std::runtime_error("Failed to set serial port state");
    }

    return hSerial;
}

bool writeData(HANDLE handle, void* data, size_t size)
{
    DWORD bytesWritten;

    if (!WriteFile(handle, data, size, &bytesWritten, NULL)) 
    {
        std::cerr << "Error writing\n";
        return false;
    }

    return true; 
}

