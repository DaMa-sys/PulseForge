#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/PatternData.h"
#include "../Source/HardwareControllerSerial.h"

// Stress test per l'Audio Thread: simula blocchi audio a 64, 128 e 256 campioni
// mentre un thread concorrente bombarda la coda con raffiche di 1000 messaggi seriali.
int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "STRESS TEST CONCORRENZA AUDIO THREAD (BUFFER 64, 128, 256 CAMPIONI)" << std::endl;
    std::cout << "================================================================================" << std::endl;

    HardwareControllerSerial serial;

    // Buffer sizes tipici in Ableton Live
    int bufferSizes[] = { 64, 128, 256 };
    const double sampleRate = 44100.0;

    for (int bufferSize : bufferSizes) {
        std::cout << "\n>>> Test con Audio Buffer = " << bufferSize << " campioni ("
                  << std::fixed << std::setprecision(2) << (bufferSize * 1000.0 / sampleRate) << " ms budget per blocco)..." << std::endl;

        std::atomic<bool> stressRunning { true };
        std::atomic<int> messagesSent { 0 };

        // Thread di background che simula l'alluvione da Arduino (1000 messaggi seriali)
        std::thread floodThread([&]() {
            int counter = 0;
            while (stressRunning.load()) {
                char buf[32];
                snprintf(buf, sizeof(buf), "T0:DENS:%d\n", counter % 100);
                // Trigger flash e comandi
                serial.sendStepFlash(0, counter % 16, (counter % 4 == 0));
                messagesSent.fetch_add(1, std::memory_order_relaxed);
                counter++;
                std::this_thread::yield();
            }
        });

        // Loop audio real-time simulato (1000 blocchi consecutivi)
        const int kNumBlocks = 1000;
        int xrunCount = 0;
        double maxBlockTimeMs = 0.0;
        const double budgetMs = bufferSize * 1000.0 / sampleRate;

        for (int b = 0; b < kNumBlocks; ++b) {
            auto start = std::chrono::high_resolution_clock::now();

            // Simulazione operazione audio thread: invio step flash lock-free + elaborazione eventi
            serial.sendStepFlash(0, b % 16, (b % 4 == 0));

            // Simula calcolo DSP del blocco
            std::vector<float> dummyBuffer(bufferSize, 0.0f);
            for (int i = 0; i < bufferSize; ++i) dummyBuffer[i] = dummyBuffer[i] * 0.99f + 0.001f;

            auto end = std::chrono::high_resolution_clock::now();
            double blockTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

            if (blockTimeMs > maxBlockTimeMs)
                maxBlockTimeMs = blockTimeMs;

            // XRun se il tempo impiegato supera il budget temporale del buffer audio
            if (blockTimeMs > budgetMs)
                xrunCount++;
        }

        stressRunning.store(false);
        if (floodThread.joinable()) floodThread.join();

        std::cout << "  Messaggi inviati durante il test: " << messagesSent.load() << std::endl;
        std::cout << "  Blocchi audio processati:         " << kNumBlocks << std::endl;
        std::cout << "  Tempo massimo per blocco:         " << std::fixed << std::setprecision(3) << maxBlockTimeMs << " ms" << std::endl;
        std::cout << "  XRuns / Dropouts rilevati:        " << xrunCount << " (Budget: " << budgetMs << " ms)" << std::endl;
        std::cout << "  Esito Audio Thread:               " << (xrunCount == 0 ? "PASS (Zero Dropouts, 100% Real-Time)" : "FAIL") << std::endl;
    }

    return 0;
}
