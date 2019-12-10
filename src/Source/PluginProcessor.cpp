/*********************************************************************
 
 Sprike
 https://github.com/cognitone/sprike
 
 Tunefish4
 http://tunefish-synth.com
 
 This file is part of Sprike, Cognitone (2017)
 An extended version of Tunefish4, Brain Control (2014)
 
 Sprike is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Sprike is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Sprike. If not, see <http://www.gnu.org/licenses/>.
 
 *********************************************************************/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "tfextensions.h"


/** 
 Preset folder where programs are saved. Read-only factory banks are stored in a protected
 system area, while all sounds saved by the user are stored in a shared user's directory. 
 User files take precedence over factory presets when loading a program.
 */

static File presetsDirectory (bool factory)
{
    String folder (JucePlugin_Manufacturer);
    folder << File::getSeparatorString() << JucePlugin_Name;
#if JUCE_MAC
    if (factory)
        return File(String("/Library/Audio/Presets/") + folder);
    else
        return File::getSpecialLocation(File::userHomeDirectory)
        .getChildFile(String("Library/Audio/Presets/") + folder);
#elif JUCE_WINDOWS
    if (factory)
        // C:\ProgramData\<JucePlugin_Manufacturer>\<JucePlugin_Name>
        return File::getSpecialLocation(File::commonApplicationDataDirectory)
        .getChildFile(folder);
    else
        // C:\Users\<Username>\AppData\Local\<JucePlugin_Manufacturer>\<JucePlugin_Name>
        return File::getSpecialLocation(File::userApplicationDataDirectory)
        .getChildFile(folder);
#else
#error You must add support for your OS here!
#endif
}


//==============================================================================

PluginProcessor::PluginProcessor() :
    tf(nullptr),
    synth(nullptr),
    paramDirtyAny(false),
    currentProgramIndex(0),
    currentProgram(new eTfSynthProgram()),
    adapterWriteOffset(0),
    adapterDataAvailable(0),
    masterGain(1.0),
    masterPan(0.5),
    requestedBank_MSB(0),
    requestedBank_LSB(0),
    requestedPreset(0)
{
    meterLevels[0] = 0;
    meterLevels[1] = 0;
    metering.set(0);

    adapterBuffer[0] = new eF32[TF_BUFFERSIZE];
    adapterBuffer[1] = new eF32[TF_BUFFERSIZE];

    synth = new eTfSynth();
    eTfSynthInit(*synth);
    synth->sampleRate = 44100;

    synth->instr[0] = tf = new eTfInstrument();
    eTfInstrumentInit(*synth, *tf);

    for (auto i=0; i < TF_PLUG_NUM_PROGRAMS; i++)
    {
        programs[i].loadDefault(i);
        privateLoadProgram(i);
    }
    programs[currentProgramIndex].applyToSynth(tf);
    resetParamDirty(true);
    
    addChangeListener(this);
}

PluginProcessor::~PluginProcessor()
{
    removeChangeListener(this);
    eDelete(adapterBuffer[0]);
    eDelete(adapterBuffer[1]);
    eDelete(tf);
    eDelete(synth);
}

//==============================================================================

float PluginProcessor::delayFromGrid (int paramIndex, float paramGridValue, double bpm)
{
    // Return delay parameter (0..1 mapping to 0..TF_FX_DELAY_MAX_MILLISECONDS)
    // depending on current tempo & grid setting
    
    eASSERT(paramIndex == TF_DELAY_LEFT || paramIndex == TF_DELAY_RIGHT);
    
    const int grid = round(TF_NUM_DELAY_GRIDS * paramGridValue);
    if (grid == 0) // free
        return tf->params[paramIndex];
    
    if (bpm == 0)
    {
        AudioPlayHead::CurrentPositionInfo pos;
        getPlayHead()->getCurrentPosition(pos);
        bpm = pos.bpm;
    }
    float ms = 60000.0f * 4.0f / (bpm * DelayGridUnits[grid]);
    return ms / TF_FX_DELAY_MAX_MILLISECONDS;
}

void PluginProcessor::setDelaysFromTempo (double bpm)
{
    ScopedLock sl (getSynthCriticalSection());
    
    float grid = tf->params[TF_DELAY_RIGHT_GRID];
    if (grid > 0.0f)
    {
        tf->params[TF_DELAY_RIGHT] = delayFromGrid(TF_DELAY_RIGHT, grid);
        paramDirty[TF_DELAY_RIGHT] = eTRUE;
    }
    grid = tf->params[TF_DELAY_LEFT_GRID];
    if (grid > 0.0f)
    {
        tf->params[TF_DELAY_LEFT] = delayFromGrid(TF_DELAY_LEFT, grid);
        paramDirty[TF_DELAY_LEFT] = eTRUE;
    }
}


const String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

int PluginProcessor::getNumParameters()
{
    return TF_PARAM_COUNT;
}

bool PluginProcessor::isMetaParameter (int index) const
{
    // the new delay grids are meta parameters!
    return index >= TF_PARAM_COUNT - 2;
}

float PluginProcessor::getParameterMod(int index)
{
    eTfVoice *voice = tf->latestTriggeredVoice;
    if (!voice)
        return 0.0f;

    if (!voice->playing)
        return 0.0f;

    eF32 value = eTfModMatrixGet(voice->modMatrix, static_cast<eTfModMatrix::Output>(index));
    if (value == 1.0f)
        return 0.0f;

    return value;
}

float PluginProcessor::getParameter (int index)
{
    eASSERT(index >= 0 && index < TF_PARAM_COUNT);
    return tf->params[index];
}

void PluginProcessor::setParameter (int index, float newValue)
{
    ScopedLock sl (getSynthCriticalSection());
    
    eASSERT(index >= 0 && index < TF_PARAM_COUNT);
    
    tf->params[index] = newValue;
    paramDirty[index] = eTRUE;
    
    // Have delay sliders reflect grid setting
    if (index == TF_DELAY_RIGHT_GRID)
    {
        tf->params[TF_DELAY_RIGHT] = delayFromGrid(TF_DELAY_RIGHT, newValue);
        paramDirty[TF_DELAY_RIGHT] = eTRUE;
    }
    if (index == TF_DELAY_LEFT_GRID)
    {
        tf->params[TF_DELAY_LEFT] = delayFromGrid(TF_DELAY_LEFT, newValue);
        paramDirty[TF_DELAY_LEFT] = eTRUE;
    }
    // Reset delay grids to 'free' if sliders are moved manually
    if (index == TF_DELAY_RIGHT)
    {
        tf->params[TF_DELAY_RIGHT_GRID] = 0;
        paramDirty[TF_DELAY_RIGHT_GRID] = eTRUE;
    }
    
    if (index == TF_DELAY_LEFT)
    {
        tf->params[TF_DELAY_LEFT_GRID] = 0;
        paramDirty[TF_DELAY_LEFT_GRID] = eTRUE;
    }
    
    paramDirtyAny = true;
}

const String PluginProcessor::getParameterName (int index)
{
    eASSERT(index >= 0 && index < TF_PARAM_COUNT);
    return TF_NAMES[index];
}

const String PluginProcessor::getParameterText (int index)
{
    return String();
}

const String PluginProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String PluginProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool PluginProcessor::isInputChannelStereoPair (int index) const
{
    return true;
}

bool PluginProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return TF_PLUG_NUM_PROGRAMS;
}

int PluginProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void PluginProcessor::setCurrentProgram (int index)
{
    if (currentProgramIndex == index)
        return;

    eASSERT(index >= 0 && index < TF_PLUG_NUM_PROGRAMS);

    // write program from tunefish to program list before switching
    programs[currentProgramIndex].loadFromSynth(tf);
    currentProgramIndex = index;
    // load new program to into tunefish
    programs[currentProgramIndex].applyToSynth(tf);
    resetParamDirty(true);
    // required to please AU hosts:
    updateHostDisplay();
}

const String PluginProcessor::getProgramName (int index)
{
    eASSERT(index >= 0 && index < TF_PLUG_NUM_PROGRAMS);
    return programs[index].getName();
}

void PluginProcessor::changeProgramName (int index, const String& newName)
{
    eASSERT(index >= 0 && index < TF_PLUG_NUM_PROGRAMS);
    programs[index].setName(newName);
}

//==============================================================================

float PluginProcessor::getMasterVolume()
{
    // returns fader position 0..1
    return convertGainToFader6dB(masterGain.get());
}

void PluginProcessor::setMasterVolume(float newValue)
{
    // takes fader position 0..1
    masterGain.set(convertFaderToGain6dB(jlimit (0.0f, 1.0f, newValue)));
}

float PluginProcessor::getMasterPan()
{
    // returns fader position 0..1, 0.5=center
    return masterPan.get();
}

void PluginProcessor::setMasterPan(float newValue)
{
    // takes fader position 0..1
    masterPan.set(jlimit (0.0f, 1.0f, newValue));
}

void PluginProcessor::setMetering (bool on)
{
    metering.set(on);
}

float PluginProcessor::getMeterLevel (int channel, int meter)
{
    return meterLevels[channel].get();
}

void PluginProcessor::changeListenerCallback (ChangeBroadcaster* source)
{
    setCurrentProgram(requestedPreset.get());
}

void PluginProcessor::processMidiVolume (int controllerValue)
{
    setMasterVolume(controllerValue / 127.0f);
}

void PluginProcessor::processMidiPan (int controllerValue)
{
    setMasterPan(controllerValue / 127.0f);
}


//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (sampleRate > 0)
        synth->sampleRate = sampleRate;
    
    setDelaysFromTempo();
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void PluginProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);
    
    MidiBuffer::Iterator it(midiMessages);
    MidiMessage midiMessage;
    eU32 messageOffset = 0;
    eU32 requestedLen = buffer.getNumSamples();

    eU32 sampleRate = static_cast<eU32>(getSampleRate());
    if (sampleRate > 0)
        synth->sampleRate = sampleRate;

    for (int i = 0; i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    if (buffer.getNumChannels() == 2)
    {
        eU32 len = requestedLen;
        eF32 **signal = buffer.getArrayOfWritePointers();
        eF32 *destL = signal[0];
        eF32 *destR = signal[1];

        while(len)
        {
            if (!adapterDataAvailable)
            {
                csSynth.enter();
                eMemSet(adapterBuffer[0], 0, TF_BUFFERSIZE * sizeof(eF32));
                eMemSet(adapterBuffer[1], 0, TF_BUFFERSIZE * sizeof(eF32));
                processEvents(midiMessages, messageOffset, TF_BUFFERSIZE);
                eTfInstrumentProcess(*synth, *tf, adapterBuffer, TF_BUFFERSIZE);
                messageOffset += TF_BUFFERSIZE;
                adapterDataAvailable = TF_BUFFERSIZE;
                csSynth.exit();
            }

            eF32 *srcL = &adapterBuffer[0][TF_BUFFERSIZE - adapterDataAvailable];
            eF32 *srcR = &adapterBuffer[1][TF_BUFFERSIZE - adapterDataAvailable];

            while (len && adapterDataAvailable)
            {
                *destL++ += *srcL++;
                *destR++ += *srcR++;

                len--;
                adapterDataAvailable--;
            }
        }
    }

    processEvents(midiMessages, messageOffset, requestedLen);
	midiMessages.clear();
    
    // Master Volume & Pan, Metering
    if (buffer.getNumChannels() == 2)
    {
        float masterPanL, masterPanR;
        convertFaderToPan(masterPan.get(), masterPanL, masterPanR);
        buffer.applyGain (0, 0, requestedLen, masterGain.get() * masterPanL);
        buffer.applyGain (1, 0, requestedLen, masterGain.get() * masterPanR);
        
        if (metering.get())
        {
            meterLevels[0].set (buffer.getMagnitude (0, 0, buffer.getNumSamples()));
            meterLevels[1].set (buffer.getMagnitude (1, 0, buffer.getNumSamples()));
        }
    }
}

void PluginProcessor::processEvents (MidiBuffer &midiMessages, eU32 messageOffset, eU32 frameSize)
{
    MidiBuffer::Iterator it(midiMessages);
    MidiMessage midiMessage;
    int samplePosition;

    it.setNextSamplePosition(messageOffset);

    while (it.getNextEvent(midiMessage, samplePosition))
    {
        if (samplePosition >= messageOffset + frameSize)
            break;

        if (midiMessage.isNoteOn())
        {
            eU8 velocity = static_cast<eU8>(midiMessage.getVelocity());
            eU8 note = static_cast<eU8>(midiMessage.getNoteNumber());

            eTfInstrumentNoteOn(*tf, note, velocity);
        }
        else if (midiMessage.isNoteOff())
        {
            eU8 note = static_cast<eU8>(midiMessage.getNoteNumber());

            eTfInstrumentNoteOff(*tf, note);
        }
        else if (midiMessage.isAllNotesOff())
        {
            eTfInstrumentAllNotesOff(*tf);
        }
        else if (midiMessage.isControllerOfType(121))
        {   // Reset-All-Controllers
            eTfInstrumentPitchBend(*tf,0,0);
            processMidiVolume(FaderPosUnity * 127);
            processMidiPan(64);
            paramDirtyAny = true;
        }
        else if (midiMessage.isPitchWheel())
        {
            eS32 bend_lsb = midiMessage.getRawData()[1] & 0x7f;
            eS32 bend_msb = midiMessage.getRawData()[2] & 0x7f;
            
            auto semitones = ((eF32(bend_msb) / 127.0f) - 0.5f) * 2.0f;
            auto cents = ((eF32(bend_lsb) / 127.0f) - 0.5f) * 2.0f;
            
            eTfInstrumentPitchBend(*tf, semitones, cents);
        }
        else if (midiMessage.isTempoMetaEvent())
        {
            setDelaysFromTempo(1.0 / (midiMessage.getTempoSecondsPerQuarterNote() / 60.0));
            paramDirtyAny = true;
        }
        else if (midiMessage.isControllerOfType(7))
        {
            processMidiVolume(midiMessage.getControllerValue());
            paramDirtyAny = true;
        }
        else if (midiMessage.isControllerOfType(10))
        {
            processMidiPan(midiMessage.getControllerValue());
            paramDirtyAny = true;
        }
        else if (midiMessage.isControllerOfType(0))
        {
            requestedBank_MSB = midiMessage.getControllerValue();
        }
        else if (midiMessage.isControllerOfType(32))
        {
            requestedBank_LSB = midiMessage.getControllerValue();
        }
        else if (midiMessage.isProgramChange())
        {
            requestedPreset.set(jmin((int)TF_PLUG_NUM_PROGRAMS-1,
                                     (((requestedBank_MSB * 128) + requestedBank_LSB) * 128 + midiMessage.getProgramChangeNumber())));
            sendChangeMessage();
        }
        else if (midiMessage.isControllerOfType(35))
        {   // Alternative for Program Selection where only CC control is available
            requestedPreset.set(jmin((int)TF_PLUG_NUM_PROGRAMS-1,
                                     (((requestedBank_MSB * 128) + requestedBank_LSB) * 128 + midiMessage.getControllerValue())));
            sendChangeMessage();
        }
    }
}


void PluginProcessor::presetSave()
{
    programs[currentProgramIndex].loadFromSynth(tf);
    privateSaveProgram(currentProgramIndex);
}

void PluginProcessor::presetCopy()
{
    // copies current synth parameters w/o saving a preset yet
    clipboard = new eTfSynthProgram();
    clipboard->setName(programs[currentProgramIndex].getName());
    clipboard->loadFromSynth(tf);
}

void PluginProcessor::presetPaste()
{
    if (clipboard == nullptr)
        return;
    
    programs[currentProgramIndex] = *clipboard;
    programs[currentProgramIndex].setName(clipboard->getName());
    programs[currentProgramIndex].applyToSynth(tf);
    // paste is persistent: write through to disk
    privateSaveProgram(currentProgramIndex);
    resetParamDirty(true);
}

void PluginProcessor::presetRestore()
{
    // revert to last saved program
    privateLoadProgram(currentProgramIndex);
    programs[currentProgramIndex].applyToSynth(tf);
    resetParamDirty(true);
    updateHostDisplay();
}


bool PluginProcessor::privateLoadProgram (eU32 index)
{
    // For backwards compatibility, look for old Tunefish4 files first
    // These will be renamed and moved to the new bank folders automatically
    bool imported = true;
    File file = presetsDirectory(false)
    .getChildFile(String("tf4programs") + File::getSeparatorString() +
                  String("program") + String(index) + String(".txt"));
    
    if (!file.existsAsFile())
    {
        imported = false;
        // Look in new per-bank structure
        const int bank = index / 128;
        const int prog = index % 128;
        String slot = String("bank") + String(bank) + File::getSeparatorString()
        + String("program") + String(prog).paddedLeft('0',3) + String(".txt");
        
        // Look in user folder first, then in factory presets
        file = presetsDirectory(false).getChildFile(slot);
        if (!file.existsAsFile())
            file = presetsDirectory(true).getChildFile(slot);
        
    }
    if (!file.existsAsFile())
        return false;
    
    ScopedPointer<FileInputStream> stream = file.createInputStream();
    if (stream == nullptr)
    {
        NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::WarningIcon,
                                         "Error",
                                         "Failed opening " + file.getFullPathName());
        return false;
    }
    DBG ("Loading " << file.getFullPathName());
    programs[index].loadDefault(index);
    programs[index].setName(stream->readNextLine());
    
    while(true)
    {
        String line = stream->readNextLine();
        if (line.length() == 0)
            break;
        
        StringArray parts;
        parts.addTokens(line, ";", "");
        if (parts.size() == 2)
        {
            String key = parts[0];
            eF32 value = parts[1].getFloatValue();
            
            for(eU32 i=0;i<TF_PARAM_COUNT;i++)
            {
                if (key == TF_NAMES[i])
                {
                    programs[index].setParam(i, value);
                    break;
                }
            }
        }
    }
    
    // Get rid of the imported file and save in new structure
    if (imported)
    {
        stream = nullptr;
        if (privateSaveProgram(index))
            file.deleteFile();
        DBG("   migrated to: " << file.getFullPathName());
    }
    return true;
}


bool PluginProcessor::privateSaveProgram (eU32 index)
{
    // Files are saved in shared users directory, preserving the factory presets
    const int bank = index / 128;
    const int prog = index % 128;
    
    File file = presetsDirectory(false).getChildFile(
        String("bank") + String(bank) + File::getSeparatorString() +
        String("program") + String(prog).paddedLeft('0',3) + String(".txt"));

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    ScopedPointer<FileOutputStream> stream = file.createOutputStream();
    if (stream == nullptr)
    {
        NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::WarningIcon,
                                         "Error",
                                         "Failed writing " + file.getFullPathName());
        return false;
    }
    DBG ("Saving " << file.getFullPathName());
    stream->writeText(programs[index].getName(), false, false, nullptr);
    stream->writeText("\r\n", false, false, nullptr);

    for(eU32 i=0; i<TF_PARAM_COUNT; i++)
    {
        stream->writeText(TF_NAMES[i], false, false, nullptr);
        stream->writeText(";", false, false, nullptr);
        stream->writeText(String(programs[index].getParam(i)), false, false, nullptr);
        stream->writeText("\r\n", false, false, nullptr);
    }
    return true;
}



bool PluginProcessor::isParamDirtyAny()
{
    return paramDirtyAny;
}

bool PluginProcessor::isParamDirty(eU32 index)
{
    return paramDirty[index];
}

bool PluginProcessor::wasProgramSwitched() const
{
    return programSwitched;
}

void PluginProcessor::resetParamDirty(eBool dirty)
{
    for (eU32 j = 0; j<TF_PARAM_COUNT; j++)
    {
        paramDirty[j] = dirty;
    }

    programSwitched = dirty;
    paramDirtyAny = dirty;
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (this, synth);
}

//==============================================================================
void PluginProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml ("SYNTH-PARAMETERS");

    for (eU32 i=0; i<TF_PARAM_COUNT; i++)
    {
        xml.setAttribute (TF_NAMES[i], tf->params[i]);
    }
    xml.setAttribute ("MasterVolume", getMasterVolume());
    xml.setAttribute ("MasterPan", getMasterPan());
    copyXmlToBinary (xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("SYNTH-PARAMETERS"))
        {
            for (eU32 i=0; i<TF_PARAM_COUNT; i++)
            {
                tf->params[i] = static_cast<float>(xmlState->getDoubleAttribute (TF_NAMES[i], tf->params[i]));
            }
            setMasterVolume(static_cast<float>(xmlState->getDoubleAttribute("MasterVolume", FaderPosUnity)));
            setMasterPan(static_cast<float>(xmlState->getDoubleAttribute("MasterPan", 0.5)));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

CriticalSection & PluginProcessor::getSynthCriticalSection()
{
    return csSynth;
}
