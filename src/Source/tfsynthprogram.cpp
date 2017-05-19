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

#define eVSTI

#include "runtime/system.hpp"
#include "tfsynthprogram.hpp"

eTfSynthProgram::eTfSynthProgram()
{
}


eTfSynthProgram::eTfSynthProgram(const eTfSynthProgram& copy) noexcept
    : name (copy.getName())
{
    for (int i=0; i < TF_PARAM_COUNT; i++)
        if (copy.hasParam(i))
            params.set(i, copy.getParam(i));
}

eTfSynthProgram& eTfSynthProgram::operator= (const eTfSynthProgram& copy)
{
    name = copy.getName();
    params.clear();
    for (int i=0; i < TF_PARAM_COUNT; i++)
        if (copy.hasParam(i))
            params.set(i, copy.getParam(i));
    return *this;
}

void eTfSynthProgram::loadDefault (int i)
{
    name = String("INIT " + String(i));
    params.clear();
    for (int i=0; i < TF_PARAM_COUNT; i++)
        params.set(i, TF_DEFAULTPROG[i]);
}


void eTfSynthProgram::applyToSynth (eTfInstrument* inst)
{
    for (int i=0; i < TF_PARAM_COUNT; i++)
        inst->params[i] = getParam(i);
}

void eTfSynthProgram::loadFromSynth (eTfInstrument* inst)
{
    for (int i=0; i < TF_PARAM_COUNT; i++)
        params.set(i, inst->params[i]);
}


void eTfSynthProgram::setParam (eU32 index, eF32 value)
{
    params.set(index,value);
}

eF32 eTfSynthProgram::getParam (eU32 index) const
{
    if (params.contains(index))
        return params[index];
    else
        return 0.0f;
}

bool eTfSynthProgram::hasParam (eU32 index) const
{
    return params.contains(index);
}

String eTfSynthProgram::getName() const
{
    return name;
}

void eTfSynthProgram::setName (const juce::String newName)
{
    name = newName;
}



