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

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#define eVSTI

#include "../JuceLibraryCode/JuceHeader.h"

#include "tflookandfeel.h"
#include "runtime/system.hpp"
#include "tfsynthprogram.hpp"
#include "synth/tf4.hpp"

const eU32 TF_PLUG_NUM_PROGRAMS = 1024;



class PluginProcessor  :
    public AudioProcessor,
    public ChangeBroadcaster,
    public ChangeListener,
    public LevelMeterSource
{
public:
     PluginProcessor();
    ~PluginProcessor();

    void                    prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void                    releaseResources() override;

    void                    processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override;
    void                    processEvents(MidiBuffer &midiMessages, eU32 messageOffset, eU32 frameSize);


    AudioProcessorEditor*   createEditor() override;
    bool                    hasEditor() const override;

    const String            getName() const override;

    int                     getNumParameters() override;
    bool                    isMetaParameter (int index) const override;

    float                   getParameterMod(int index);

    float                   getParameter (int index) override;
    void                    setParameter (int index, float newValue) override;

    const String            getParameterName (int index) override;
    const String            getParameterText (int index) override;

    const String            getInputChannelName (int channelIndex) const override;
    const String            getOutputChannelName (int channelIndex) const override;
    bool                    isInputChannelStereoPair (int index) const override;
    bool                    isOutputChannelStereoPair (int index) const override;

    bool                    acceptsMidi() const override;
    bool                    producesMidi() const override;
    bool                    silenceInProducesSilenceOut() const override;
    double                  getTailLengthSeconds() const override;

    int                     getNumPrograms() override;
    int                     getCurrentProgram() override;
    void                    setCurrentProgram (int index) override;
    const String            getProgramName (int index) override;
    void                    changeProgramName (int index, const String& newName) override;

    CriticalSection &       getSynthCriticalSection();

    void                    getStateInformation (MemoryBlock& destData) override;
    void                    setStateInformation (const void* data, int sizeInBytes) override;

    // User interface button actions:
    void                    presetSave();
    void                    presetCopy();
    void                    presetPaste();
    void                    presetRestore();

    bool                    isParamDirty(eU32 index);
    bool                    isParamDirtyAny();
    void                    resetParamDirty(eBool dirty = eFALSE);
    bool                    wasProgramSwitched() const;
    
    void                    setDelaysFromTempo(double bpm = 0);
    
    MidiKeyboardState       keyboardState;
    
    float                   getMasterVolume();
    void                    setMasterVolume(float newValue);
    float                   getMasterPan();
    void                    setMasterPan(float newValue);
    void                    setMetering (bool on);
    float                   getMeterLevel (int channel, int meter = 0) override;
    
private:
    //==============================================================================
    
    bool                    privateLoadProgram(eU32 index);
    bool                    privateSaveProgram(eU32 index);

    eTfInstrument *         tf;
    eTfSynth *              synth;
    eTfSynthProgram         programs[TF_PLUG_NUM_PROGRAMS]; 
    eBool                   paramDirty[TF_PARAM_COUNT];
    eBool                   paramDirtyAny;
    eBool                   programSwitched;
    eU32                    currentProgramIndex;

    ScopedPointer<eTfSynthProgram> currentProgram;
    ScopedPointer<eTfSynthProgram> clipboard;
    
    CriticalSection         csSynth;

    eF32 *                  adapterBuffer[2];
    eU32                    adapterWriteOffset;
    eU32                    adapterDataAvailable;
    
    void                    processMidiPan(int controllerValue);
    void                    processMidiVolume(int controllerValue);
    void                    changeListenerCallback (ChangeBroadcaster* source) override;
    float                   delayFromGrid (int paramIndex, float paramGridValue, double bpm = 0);
    
    // Parameters asynchronously accessed by UI must be Atomic!
    Atomic<float>           masterGain;
    Atomic<float>           masterPan;
    int                     requestedBank_MSB;
    int                     requestedBank_LSB;
    Atomic<int>             requestedPreset;
    Atomic<float>           meterLevels[2];
    Atomic<int>             metering;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
