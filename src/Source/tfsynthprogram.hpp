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

#ifndef TF_SYNTHPROGRAM_HPP
#define TF_SYNTHPROGRAM_HPP

#include "../JuceLibraryCode/JuceHeader.h"
#include "synth/tf4.hpp"


class eTfSynthProgram
{
public:
    
    typedef juce::HashMap<int,eF32> Parameters;
    
    eTfSynthProgram();
    eTfSynthProgram(const eTfSynthProgram& copy) noexcept; // simple copy
    eTfSynthProgram& operator=(const eTfSynthProgram& copy); // copy assignemt

    String   getName() const;
    void     setName(const juce::String newName);
    void     setParam(eU32 index, eF32 value);
    eF32     getParam(eU32 index) const;
    bool     hasParam(eU32 index) const;
    
	void     loadDefault(int i);
    void     applyToSynth (eTfInstrument* inst);
    void     loadFromSynth (eTfInstrument* inst);
    
private:
    Parameters params;
    String     name;
    
    JUCE_LEAK_DETECTOR (eTfSynthProgram);
};

#endif
