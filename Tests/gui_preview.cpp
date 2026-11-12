#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    PulseForgeAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditorIfNeeded());
    if (editor != nullptr)
    {
        editor->setSize (1180, 528);
        editor->setVisible (true);

        juce::Image snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        
        juce::File outputFile (argc > 1 ? argv[1] : "C:/vst plugin/DaMA/PulseForge/pulseforge_preview.png");
        outputFile.getParentDirectory().createDirectory();
        outputFile.deleteFile();

        juce::FileOutputStream stream (outputFile);
        if (stream.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream (snapshot, stream);
            std::cout << "SUCCESS: GUI preview written to: " << outputFile.getFullPathName() << std::endl;
            return 0;
        }
    }

    std::cerr << "ERROR: Failed to render GUI preview" << std::endl;
    return 1;
}
