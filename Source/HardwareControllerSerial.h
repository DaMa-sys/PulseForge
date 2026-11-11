#pragma once
#include <juce_core/juce_core.h>
#include <windows.h>
#include <atomic>
#include <thread>
#include <functional>
#include <vector>

//==============================================================================
// HardwareControllerSerial: Win32 background thread that handles Auto-Discovery
// and non-blocking serial communication with the DaMA PulseForge Arduino Controller.
//==============================================================================
class HardwareControllerSerial
{
public:
    HardwareControllerSerial() = default;

    ~HardwareControllerSerial()
    {
        closePort();
    }

    // Callbacks from Hardware
    std::function<void(int track, const juce::String& param, int value)> onParameterChange;
    std::function<void(int track)> onGenerateTrigger;
    std::function<void(int track)> onMutateTrigger;
    std::function<void(int track, const juce::String& target, int dir)> onEncoderStep;
    std::function<void(bool isConnected, const juce::String& portName)> onConnectionStatusChanged;

    bool isConnected() const noexcept { return connected.load(); }
    juce::String getConnectedPortName() const { return currentPort; }

    void startAutoConnect()
    {
        if (workerRunning.load())
            return;

        workerRunning.store (true);
        workerThread = std::thread ([this]() { runWorker(); });
    }

    void stopAutoConnect()
    {
        workerRunning.store (false);
        if (workerThread.joinable())
            workerThread.join();
        closePort();
    }

    // Send formatted commands to hardware
    void sendStepFlash (int trackIdx, int stepIdx, bool isAccent)
    {
        if (!connected.load() || hComm == INVALID_HANDLE_VALUE)
            return;

        juce::String msg = "STEP:T" + juce::String (trackIdx) + ":" + juce::String (stepIdx);
        if (isAccent) msg += ":ACC";
        msg += "\n";
        writeSerialString (msg);
    }

    void sendOledLine (int trackIdx, int lineIdx, const juce::String& text)
    {
        if (!connected.load() || hComm == INVALID_HANDLE_VALUE)
            return;

        juce::String msg = "OLED:T" + juce::String (trackIdx) + ":L" + juce::String (lineIdx) + ":" + text + "\n";
        writeSerialString (msg);
    }

private:
    std::atomic<bool> workerRunning { false };
    std::atomic<bool> connected { false };
    std::thread workerThread;
    HANDLE hComm = INVALID_HANDLE_VALUE;
    juce::String currentPort = "";

    void closePort()
    {
        if (hComm != INVALID_HANDLE_VALUE)
        {
            CloseHandle (hComm);
            hComm = INVALID_HANDLE_VALUE;
        }
        if (connected.exchange (false))
        {
            if (onConnectionStatusChanged)
                onConnectionStatusChanged (false, "");
        }
    }

    bool openPort (const juce::String& portName)
    {
        closePort();

        juce::String winPort = "\\\\.\\" + portName;
        hComm = CreateFileA (winPort.toRawUTF8(), GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 0, NULL);

        if (hComm == INVALID_HANDLE_VALUE)
            return false;

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof (dcbSerialParams);

        if (!GetCommState (hComm, &dcbSerialParams))
        {
            CloseHandle (hComm);
            hComm = INVALID_HANDLE_VALUE;
            return false;
        }

        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity   = NOPARITY;
        dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

        if (!SetCommState (hComm, &dcbSerialParams))
        {
            CloseHandle (hComm);
            hComm = INVALID_HANDLE_VALUE;
            return false;
        }

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 10;
        timeouts.ReadTotalTimeoutConstant = 30;
        timeouts.ReadTotalTimeoutMultiplier = 2;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 5;
        SetCommTimeouts (hComm, &timeouts);

        return true;
    }

    bool writeSerialString (const juce::String& str)
    {
        if (hComm == INVALID_HANDLE_VALUE)
            return false;

        DWORD bytesWritten = 0;
        auto raw = str.toRawUTF8();
        auto len = static_cast<DWORD> (strlen (raw));
        return WriteFile (hComm, raw, len, &bytesWritten, NULL) && (bytesWritten == len);
    }

    juce::StringArray enumerateSerialPorts()
    {
        juce::StringArray usbPorts;
        juce::StringArray otherPorts;

        // 1. First inspect Windows Registry for active hardware COM ports
        HKEY hKey;
        if (RegOpenKeyExA (HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char valName[256];
            BYTE valData[256];
            DWORD valIndex = 0;

            while (true)
            {
                DWORD nameLen = sizeof (valName);
                DWORD dataLen = sizeof (valData);
                DWORD type = 0;

                LONG res = RegEnumValueA (hKey, valIndex++, valName, &nameLen, NULL, &type, valData, &dataLen);
                if (res != ERROR_SUCCESS)
                    break;

                juce::String keyName (valName);
                juce::String portName (reinterpret_cast<char*> (valData));

                // Ignore Bluetooth serial ports to avoid 15-30s Windows blocking timeouts
                if (keyName.containsIgnoreCase ("BthModem") || keyName.containsIgnoreCase ("Bluetooth"))
                    continue;

                if (keyName.containsIgnoreCase ("USBSER") || keyName.containsIgnoreCase ("VCP") ||
                    keyName.containsIgnoreCase ("USB")    || keyName.containsIgnoreCase ("FTDI"))
                {
                    usbPorts.addIfNotAlreadyThere (portName);
                }
                else
                {
                    otherPorts.addIfNotAlreadyThere (portName);
                }
            }
            RegCloseKey (hKey);
        }

        // Combine: USB ports first (e.g. Arduino Mega COM8), then standard hardware COM ports
        juce::StringArray result;
        for (const auto& p : usbPorts)
            result.add (p);
        for (const auto& p : otherPorts)
            result.add (p);

        // Fallback: QueryDosDevice if registry had no entries
        if (result.isEmpty())
        {
            char targetPath[1024];
            for (int i = 1; i <= 32; ++i)
            {
                juce::String name = "COM" + juce::String (i);
                if (QueryDosDeviceA (name.toRawUTF8(), targetPath, sizeof (targetPath)) != 0)
                    result.add (name);
            }
        }

        return result;
    }

    void runWorker()
    {
        char rxBuf[128];
        int rxPos = 0;

        while (workerRunning.load())
        {
            // 1. If not connected, scan ports for Auto-Discovery Handshake
            if (!connected.load())
            {
                auto availablePorts = enumerateSerialPorts();
                for (const auto& port : availablePorts)
                {
                    if (!workerRunning.load()) break;

                    if (openPort (port))
                    {
                        // Wait for Arduino bootloader to complete if reset by DTR
                        std::this_thread::sleep_for (std::chrono::milliseconds (1600));

                        // Clean serial buffer
                        PurgeComm (hComm, PURGE_RXCLEAR | PURGE_TXCLEAR);

                        // Send Handshake Ping
                        writeSerialString ("PF_PING?\n");

                        // Read response over up to 1.8s looking for PF_PONG
                        juce::String accum;
                        bool foundPong = false;
                        auto startTime = std::chrono::steady_clock::now();
                        int pingRetryCounter = 0;

                        while (std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - startTime).count() < 1800)
                        {
                            char chunk[128] = { 0 };
                            DWORD bytesRead = 0;
                            if (ReadFile (hComm, chunk, sizeof (chunk) - 1, &bytesRead, NULL) && bytesRead > 0)
                            {
                                chunk[bytesRead] = '\0';
                                accum += juce::String (chunk);

                                if (accum.contains ("PF_PONG:PulseForge_Controller"))
                                {
                                    foundPong = true;
                                    break;
                                }

                                if (accum.length() > 512)
                                    accum = accum.substring (accum.length() - 256);
                            }

                            std::this_thread::sleep_for (std::chrono::milliseconds (30));

                            // Periodically re-ping every ~450ms in case first ping was dropped during setup
                            if (++pingRetryCounter >= 15)
                            {
                                pingRetryCounter = 0;
                                writeSerialString ("PF_PING?\n");
                            }
                        }

                        if (foundPong)
                        {
                            connected.store (true);
                            currentPort = port;
                            if (onConnectionStatusChanged)
                                onConnectionStatusChanged (true, port);
                            break;
                        }
                        else
                        {
                            closePort();
                        }
                    }
                }

                if (!connected.load())
                    std::this_thread::sleep_for (std::chrono::milliseconds (1000));
            }
            else
            {
                // 2. Connected: Process incoming hardware telemetry
                char byteIn = 0;
                DWORD bytesRead = 0;

                if (ReadFile (hComm, &byteIn, 1, &bytesRead, NULL) && bytesRead == 1)
                {
                    if (byteIn == '\n' || byteIn == '\r')
                    {
                        if (rxPos > 0)
                        {
                            rxBuf[rxPos] = '\0';
                            parseIncomingMessage (juce::String (rxBuf));
                            rxPos = 0;
                        }
                    }
                    else if (rxPos < (int)sizeof (rxBuf) - 1)
                    {
                        rxBuf[rxPos++] = byteIn;
                    }
                }
                else
                {
                    // Check if device was disconnected
                    DWORD commErrors;
                    COMSTAT commStat;
                    if (!ClearCommError (hComm, &commErrors, &commStat))
                    {
                        closePort();
                    }
                    else
                    {
                        std::this_thread::sleep_for (std::chrono::milliseconds (2));
                    }
                }
            }
        }
    }

    void parseIncomingMessage (const juce::String& msg)
    {
        // Format examples: "T0:GEN", "T1:MUT", "T0:DENS:65", "T2:ENC:STEPS:+1"
        if (msg.startsWith ("T") && msg.length() >= 5)
        {
            int track = msg.substring (1, 2).getIntValue();
            auto rest = msg.substring (3);

            if (rest == "GEN")
            {
                if (onGenerateTrigger) onGenerateTrigger (track);
            }
            else if (rest == "MUT")
            {
                if (onMutateTrigger) onMutateTrigger (track);
            }
            else if (rest.startsWith ("ENC:"))
            {
                auto parts = juce::StringArray::fromTokens (rest.substring (4), ":", "");
                if (parts.size() >= 2 && onEncoderStep)
                    onEncoderStep (track, parts[0], parts[1].getIntValue());
            }
            else
            {
                auto parts = juce::StringArray::fromTokens (rest, ":", "");
                if (parts.size() >= 2 && onParameterChange)
                    onParameterChange (track, parts[0], parts[1].getIntValue());
            }
        }
    }
};
