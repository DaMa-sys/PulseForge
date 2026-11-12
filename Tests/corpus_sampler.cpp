#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <set>
#include <random>
#include <algorithm>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

struct MidiFileInfo
{
    juce::String fullPath;
    juce::String fileName;
    juce::String role; // "ARP", "BASS", "CHORDS"
    int noteCount = 0;
    double durationTicks = 0.0;
    double avgPitch = 0.0;
    double maxPolyphony = 1.0;
    uint32_t contentHash = 0;
};

uint32_t computeNoteHash (const juce::MidiMessageSequence& seq)
{
    uint32_t h = 0x811c9dc5;
    for (int i = 0; i < seq.getNumEvents(); ++i)
    {
        auto msg = seq.getEventPointer (i)->message;
        if (msg.isNoteOn())
        {
            uint32_t note = static_cast<uint32_t> (msg.getNoteNumber());
            h = (h ^ note) * 0x01000193;
            uint32_t tick = static_cast<uint32_t> (msg.getTimeStamp() / 24.0); // quantize to 16ths
            h = (h ^ tick) * 0x01000193;
        }
    }
    return h;
}

int main()
{
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "CAMPIONAMENTO CORPUS MELODIC TECHNO (PAUL KALKBRENNER / AFTERLIFE / PML)" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    // Cartelle target dirette per Melodic Techno / Techno & Progressive
    const std::vector<juce::String> targetRoots = {
        "G:/samples/MIDI/PML - Midi Complete Pack/PML - Melodic Techno Melody Arps - MIDI Arps (Version 1.0)",
        "G:/samples/MIDI/PML_MIDI_PROGRESSION_PACK",
        "G:/samples/MIDI/PML - Midi Complete Pack/PML MIDI Pack V2",
        "G:/samples/MIDI/PML - Midi Complete Pack/PML_MIDI_PROGRESSIONS_PACK_V2",
        "G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Arps",
        "G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Basslines",
        "G:/samples/MIDI/ElectroniSounds - Absolute MIDI/Synth Leads & Riffs",
        "G:/samples/MIDI/Piano For Producers - Niko's Ultimate Dance Midi Pack/Niko Ultimate Dance Chords/C Major - A Minor/Trance & Techno",
        "G:/samples/MIDI/Piano For Producers - Niko's Ultimate Dance Midi Pack/Niko Ultimate Dance Chords/F Major - D Minor/Trance & Techno",
        "G:/samples/MIDI/Piano For Producers - Niko's Ultimate Dance Midi Pack/Niko Ultimate Dance Chords/G Major - E Minor/Trance & Techno",
        "G:/samples/MIDI/HexLoops.EDM.MIDI.Ultimate.1000/EDM ARPS MIDI",
        "G:/samples/MIDI/HexLoops.EDM.MIDI.Ultimate.1000/EDM BASS MIDI",
        "G:/samples/MIDI/Equinox.Sounds.Collection.MIDI/Equinox.Sounds.Trance.Progressive.MIDIs.One.Shots"
    };

    std::vector<MidiFileInfo> arpList;
    std::vector<MidiFileInfo> bassList;
    std::vector<MidiFileInfo> chordList;
    std::set<uint32_t> seenHashes;

    std::cout << "Scansione directory target in sola lettura..." << std::endl;

    for (const auto& rootPath : targetRoots)
    {
        juce::File rootDir (rootPath);
        if (!rootDir.exists())
            continue;

        juce::Array<juce::File> foundFiles;
        rootDir.findChildFiles (foundFiles, juce::File::findFiles, true, "*.mid");

        for (const auto& f : foundFiles)
        {
            // Filtro dimensione: file validi tra 100 byte e 500 KB
            const auto fSize = f.getSize();
            if (fSize < 100 || fSize > 500000)
                continue;

            // Filtro path per cartelle generiche Niko: prendere solo le sottocartelle Trance & Techno
            juce::String fullPathNorm = f.getFullPathName().replaceCharacter ('\\', '/');
            if (fullPathNorm.containsIgnoreCase ("Niko Ultimate Dance Chords") &&
                !fullPathNorm.containsIgnoreCase ("Trance & Techno"))
            {
                continue;
            }

            juce::FileInputStream inputStream (f);
            if (!inputStream.openedOk())
                continue;

            juce::MidiFile midiFile;
            if (!midiFile.readFrom (inputStream))
                continue;

            midiFile.convertTimestampTicksToSeconds();

            // Raggruppa tutte le note del file
            juce::MidiMessageSequence combinedSeq;
            for (int t = 0; t < midiFile.getNumTracks(); ++t)
                combinedSeq.addSequence (*midiFile.getTrack (t), 0.0);

            combinedSeq.updateMatchedPairs();

            int noteCount = 0;
            double pitchSum = 0.0;
            for (int e = 0; e < combinedSeq.getNumEvents(); ++e)
            {
                auto msg = combinedSeq.getEventPointer (e)->message;
                if (msg.isNoteOn())
                {
                    noteCount++;
                    pitchSum += msg.getNoteNumber();
                }
            }

            // Filtro: deve avere tra 6 e 600 note (esclude file quasi vuoti o stem interminabili)
            if (noteCount < 6 || noteCount > 600)
                continue;

            double avgPitch = pitchSum / noteCount;

            // Calcolo polifonia massima simultanea
            int curSimult = 0;
            int maxSimult = 1;
            for (int e = 0; e < combinedSeq.getNumEvents(); ++e)
            {
                auto msg = combinedSeq.getEventPointer (e)->message;
                if (msg.isNoteOn())
                {
                    curSimult++;
                    if (curSimult > maxSimult) maxSimult = curSimult;
                }
                else if (msg.isNoteOff())
                {
                    if (curSimult > 0) curSimult--;
                }
            }

            // Calcolo hash per dedublicazione contenuto
            uint32_t nh = computeNoteHash (combinedSeq);
            if (seenHashes.count (nh) > 0)
                continue; // Duplicato esatto scartato
            seenHashes.insert (nh);

            MidiFileInfo info;
            info.fullPath = fullPathNorm;
            info.fileName = f.getFileName();
            info.noteCount = noteCount;
            info.avgPitch = avgPitch;
            info.maxPolyphony = maxSimult;
            info.contentHash = nh;

            // Classificazione rigorosa per Ruolo
            // 1. BASS: nome contiene Bass/Sub, oppure tono medio basso (< 52) con linea prevalentemente monofonica
            bool nameIsBass = fullPathNorm.containsIgnoreCase ("Bass") || fullPathNorm.containsIgnoreCase ("Sub");
            bool nameIsArp  = fullPathNorm.containsIgnoreCase ("Arp") || fullPathNorm.containsIgnoreCase ("Lead") || fullPathNorm.containsIgnoreCase ("Riff");
            bool nameIsChord= fullPathNorm.containsIgnoreCase ("Chord") || fullPathNorm.containsIgnoreCase ("Progression") || fullPathNorm.containsIgnoreCase ("Pad");

            if (nameIsBass && maxSimult <= 2)
            {
                info.role = "BASS";
                if (bassList.size() < 1800)
                    bassList.push_back (info);
            }
            else if (nameIsArp && maxSimult <= 2)
            {
                info.role = "ARP";
                if (arpList.size() < 1800)
                    arpList.push_back (info);
            }
            else if (nameIsChord || maxSimult >= 3)
            {
                info.role = "CHORDS";
                if (chordList.size() < 1800)
                    chordList.push_back (info);
            }
            else if (avgPitch < 50.0 && maxSimult <= 2)
            {
                info.role = "BASS";
                if (bassList.size() < 1800)
                    bassList.push_back (info);
            }
            else if (maxSimult <= 2)
            {
                info.role = "ARP";
                if (arpList.size() < 1800)
                    arpList.push_back (info);
            }
        }
    }

    std::cout << "\n------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "RISULTATO CAMPIONAMENTO CORPUS (DEDUPPLICATO, CONVALIDATO E CLASSIFICATO):" << std::endl;
    std::cout << "  - File ARP    validati: " << arpList.size() << std::endl;
    std::cout << "  - File BASS   validati: " << bassList.size() << std::endl;
    std::cout << "  - File CHORDS validati: " << chordList.size() << std::endl;
    std::cout << "  - TOTALE CORPUS CAMPIONATO: " << (arpList.size() + bassList.size() + chordList.size()) << " file" << std::endl;
    std::cout << "------------------------------------------------------------------------------------------" << std::endl;

    // Salvataggio Cache su File JSON
    juce::var cacheRoot (new juce::DynamicObject());
    auto createJsonArray = [] (const std::vector<MidiFileInfo>& list) -> juce::var
    {
        juce::Array<juce::var> arr;
        for (const auto& item : list)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("file", item.fileName);
            obj->setProperty ("path", item.fullPath);
            obj->setProperty ("role", item.role);
            obj->setProperty ("notes", item.noteCount);
            obj->setProperty ("avgPitch", item.avgPitch);
            obj->setProperty ("poly", item.maxPolyphony);
            arr.add (juce::var (obj));
        }
        return juce::var (arr);
    };

    cacheRoot.getDynamicObject()->setProperty ("arp", createJsonArray (arpList));
    cacheRoot.getDynamicObject()->setProperty ("bass", createJsonArray (bassList));
    cacheRoot.getDynamicObject()->setProperty ("chords", createJsonArray (chordList));

    juce::File cacheFile ("Tests/corpus_melodic_techno_cache.json");
    cacheFile.deleteFile();
    if (auto out = std::unique_ptr<juce::FileOutputStream> (cacheFile.createOutputStream()))
    {
        juce::JSON::writeToStream (*out, cacheRoot);
        std::cout << "Cache salvata in: Tests/corpus_melodic_techno_cache.json\n" << std::endl;
    }

    // Estrazione di 10 file a caso per ciascun ruolo
    std::mt19937 rng (777); // Seed fisso per replicabilità
    auto print10Samples = [&rng] (const std::string& roleName, std::vector<MidiFileInfo>& list)
    {
        std::cout << "\n>>> 10 FILE CAMPIONE CASUALI PER IL RUOLO [" << roleName << "]:" << std::endl;
        std::cout << std::string (90, '-') << std::endl;
        std::shuffle (list.begin(), list.end(), rng);
        int count = std::min (10, static_cast<int> (list.size()));
        for (int i = 0; i < count; ++i)
        {
            std::cout << "  " << std::setw (2) << (i + 1) << ". "
                      << std::left << std::setw (45) << list[i].fileName
                      << " [Note: " << std::setw (3) << list[i].noteCount
                      << ", Pitch: " << std::setw (4) << std::fixed << std::setprecision (1) << list[i].avgPitch
                      << ", Poly: " << static_cast<int> (list[i].maxPolyphony) << "]"
                      << "\n     Path: " << list[i].fullPath << "\n";
        }
    };

    print10Samples ("ARP (Ipnotici, semicrome, Afterlife/PML)", arpList);
    print10Samples ("BASS (Pulsante, rolling techno, fondamentale solida)", bassList);
    print10Samples ("CHORDS (Progressioni minori, sospensioni pad)", chordList);

    return 0;
}
