#pragma once
#include <juce_core/juce_core.h>
#include <windows.h>
#include <atomic>
#include <thread>
#include <functional>
#include <vector>

//==============================================================================
// HardwareControllerSerial: Win32 background thread that handles Auto-Discovery
// and non-blocking serial communication with the Orage PulseForge Arduino Controller.
//==============================================================================
class HardwareControllerSerial
{
public:
    HardwareControllerSerial() = default;

    ~HardwareControllerSerial()
    {
        stopAutoConnect();
    }

    // Callbacks from Hardware
    std::function<void(int track, const juce::String& param, int value)> onParameterChange;
    std::function<void(int track)> onGenerateTrigger;
    std::function<void(int track)> onMutateTrigger;
    std::function<void(int track, const juce::String& target, int dir)> onEncoderStep;
    std::function<void(bool isConnected, const juce::String& portName)> onConnectionStatusChanged;
    std::function<void(bool isConnected, const juce::String& portName)> onProcessorConnectionStatusChanged;

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

    // Real-time lock-free queue for outgoing commands from audio thread to serial thread
    void sendStepFlash (int trackIdx, int stepIdx, bool isAccent) noexcept
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[32];
        if (isAccent)
            snprintf (buf, sizeof (buf), "STEP:T%d:%d:ACC\n", trackIdx, stepIdx);
        else
            snprintf (buf, sizeof (buf), "STEP:T%d:%d\n", trackIdx, stepIdx);

        enqueueTx (buf);
    }

    void sendTrackName (int trackIdx, const juce::String& name)
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[64];
        snprintf (buf, sizeof (buf), "OLED:T%d:NAME:%s\n", trackIdx, name.toRawUTF8());
        enqueueTx (buf);
    }

    void sendTrackSteps (int trackIdx, int steps)
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[32];
        snprintf (buf, sizeof (buf), "OLED:T%d:STP:%d\n", trackIdx, steps);
        enqueueTx (buf);
    }

    void sendTrackGenre (int trackIdx, const juce::String& genre)
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[48];
        snprintf (buf, sizeof (buf), "OLED:T%d:GEN:%s\n", trackIdx, genre.toRawUTF8());
        enqueueTx (buf);
    }

    void sendTrackRole (int trackIdx, const juce::String& role)
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[48];
        snprintf (buf, sizeof (buf), "OLED:T%d:ROL:%s\n", trackIdx, role.toRawUTF8());
        enqueueTx (buf);
    }

    void sendParameter (int trackIdx, const juce::String& paramKey, int value)
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[32];
        snprintf (buf, sizeof (buf), "OLED:T%d:%s:%d\n", trackIdx, paramKey.toRawUTF8(), value);
        enqueueTx (buf);
    }

    void sendOledLine (int trackIdx, int lineIdx, const juce::String& text)
    {
        if (!connected.load (std::memory_order_relaxed))
            return;

        char buf[64];
        snprintf (buf, sizeof (buf), "OLED:T%d:L%d:%s\n", trackIdx, lineIdx, text.toRawUTF8());
        enqueueTx (buf);
    }

private:
    struct OutgoingItem {
        char data[64];
        int len = 0;
    };

    static constexpr int kTxQueueSize = 128;
    OutgoingItem txQueue[kTxQueueSize];
    std::atomic<int> txWriteIndex { 0 };
    std::atomic<int> txReadIndex { 0 };

    void enqueueTx (const char* str) noexcept
    {
        const int w = txWriteIndex.load (std::memory_order_relaxed);
        const int nextW = (w + 1) % kTxQueueSize;
        if (nextW == txReadIndex.load (std::memory_order_acquire))
            return; // Ring buffer full, drop rather than blocking audio DSP thread!

        auto& item = txQueue[w];
        int l = 0;
        while (str[l] != '\0' && l < 63)
        {
            item.data[l] = str[l];
            ++l;
        }
        item.data[l] = '\0';
        item.len = l;

        txWriteIndex.store (nextW, std::memory_order_release);
    }

    void flushOutgoingTx()
    {
        if (connected.load (std::memory_order_relaxed) && hComm != INVALID_HANDLE_VALUE)
        {
            const int r = txReadIndex.load (std::memory_order_relaxed);
            if (r == txWriteIndex.load (std::memory_order_acquire))
                return; // Empty

            const auto& item = txQueue[r];
            if (item.len > 0)
            {
                DWORD bytesWritten = 0;
                WriteFile (hComm, item.data, static_cast<DWORD> (item.len), &bytesWritten, NULL);
            }

            txReadIndex.store ((r + 1) % kTxQueueSize, std::memory_order_release);

            // Pace outgoing bursts (~4 ms per message) so Arduino's 64-byte RX buffer never overflows during OLED I2C transfers
            if (txReadIndex.load (std::memory_order_relaxed) != txWriteIndex.load (std::memory_order_acquire))
                std::this_thread::sleep_for (std::chrono::milliseconds (4));
        }
    }
    std::atomic<bool> workerRunning { false };
    std::atomic<bool> connected { false };
    std::thread workerThread;
    HANDLE hComm = INVALID_HANDLE_VALUE;
    juce::String currentPort = "";

    void notifyConnectionStatus (bool isConn, const juce::String& port)
    {
        if (onProcessorConnectionStatusChanged)
            onProcessorConnectionStatusChanged (isConn, port);
        if (onConnectionStatusChanged)
            onConnectionStatusChanged (isConn, port);
    }

    void closePort()
    {
        if (hComm != INVALID_HANDLE_VALUE)
        {
            CloseHandle (hComm);
            hComm = INVALID_HANDLE_VALUE;
        }
        if (connected.exchange (false))
        {
            notifyConnectionStatus (false, "");
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
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutConstant = 0;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 20;
        timeouts.WriteTotalTimeoutMultiplier = 1;
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
                            notifyConnectionStatus (true, port);
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
                // 2. Connected: Flush any queued outgoing triggers to hardware first
                flushOutgoingTx();

                // Process incoming hardware telemetry in chunks
                char chunk[64];
                DWORD bytesRead = 0;

                if (ReadFile (hComm, chunk, sizeof (chunk), &bytesRead, NULL) && bytesRead > 0)
                {
                    for (DWORD i = 0; i < bytesRead; ++i)
                    {
                        char byteIn = chunk[i];
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

    // Incoming Parameter Coalescing & Rate Limiter (protects Host automation thread from flood)
    std::mutex paramMutex;
    struct ParamThrottler {
        int lastDispatchedValue = -9999;
        std::chrono::steady_clock::time_point lastDispatchTime = std::chrono::steady_clock::now() - std::chrono::milliseconds (500);
    };
    std::map<std::pair<int, juce::String>, ParamThrottler> paramThrottlers;

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
                {
                    const juce::String paramKey = parts[0];
                    const int paramVal = parts[1].getIntValue();

                    // Coalesce / rate-limit per parameter: minimum 35ms between host updates unless final value changed
                    std::lock_guard<std::mutex> lock (paramMutex);
                    auto& thr = paramThrottlers[{ track, paramKey }];
                    auto now = std::chrono::steady_clock::now();
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds> (now - thr.lastDispatchTime).count();

                    if (thr.lastDispatchedValue != paramVal && elapsedMs >= 35)
                    {
                        thr.lastDispatchedValue = paramVal;
                        thr.lastDispatchTime = now;
                        onParameterChange (track, paramKey, paramVal);
                    }
                }
            }
        }
    }
};
