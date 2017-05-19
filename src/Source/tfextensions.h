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

#ifndef _TF_EXTENSIONS_H_
#define _TF_EXTENSIONS_H_

#include "../JuceLibraryCode/JuceHeader.h"

/** Delay time presets */

static const int   TF_NUM_DELAY_GRIDS = 13;

static const char* DelayGridNames[TF_NUM_DELAY_GRIDS] =
    { "Free", "1/32", "1/24", "1/20", "1/16", "1/12", "1/10", "1/8", "1/6", "1/5", "1/4", "1/3", "1/2" };

static const float DelayGridUnits[TF_NUM_DELAY_GRIDS] =
    { 0, 32, 24, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2 };

static String delayGridMenuItems()
{
    String out;
    for (int i=0; i < TF_NUM_DELAY_GRIDS; ++i)
    {
        out << DelayGridNames[i];
        if (i < (TF_NUM_DELAY_GRIDS-1))
            out << "|";
    }
    return out;
}




/** Conversions from linear knobs/faders (0...1) to audio multipliers (gain) and vice versa .
    This is used for converting MIDI CC to parameters and back. */

/** Maps a linear knob (0..1) to u-Law pan multipliers for channels L,R. Center value is 0.5. */
static void convertFaderToPan (const float fader, float& scaleLeft, float& scaleRight)
{
    //scaleLeft = fader;
    //scaleRight = 1.0f - fader;
    float pan = juce::jlimit (0.0f, 1.0f, fader);
    scaleLeft = sqrt(1.0f - pan);
    scaleRight = sqrt(pan);
}

/** These parameters resemble a default mixing console dB scaling */
#define FaderFactorA6dB   0.002
#define FaderFactorB6dB   6.908
  
#define FaderFactorA0dB   0.0012
#define FaderFactorB0dB   6.726
  
/** Linear fader position that returns 1.0 for +6dB mapping */
#define FaderPosUnity     0.811024
#define FaderINFPos       0.006
#define FaderSnapUnity    0.0066
  
/** Maps linear fader/knob position (0..1) to gain multiplier 0.0 (-INF) ... ~2.0 (+6dB). */
static double convertFaderToGain6dB (const double fader, double faderCutOff = FaderINFPos)
{
    if (fader < faderCutOff)
      return 0.0;
    // Resembles an analog mixer fader
    double gain = FaderFactorA6dB * exp(FaderFactorB6dB * sqrt( juce::jlimit (0.0, 1.0, fader)));
    if (fabs(gain - 1.0) < FaderSnapUnity)
      return 1.0;
    return gain;
}

/** Maps a gain multiplier 0.0 (-INF) ... ~2.0 (+6dB) to linear fader/knob position (0..1) */
static double convertGainToFader6dB (const double gain, double faderCutOff = FaderINFPos)
{
    // Inverse function to the above
    double fader =
      juce::jlimit (0.0,
                    1.0,
                    pow(log(juce::jlimit(0.0, 2.0, gain) / FaderFactorA6dB) / FaderFactorB6dB,
                        2.0));
    if (fader < faderCutOff)
      return 0.0;
    return fader;
}

/** Maps linear fader/knob position (0..1) to gain multiplier 0.0 (-INF) ... 1.0 (0dB) */
static double convertFaderToGain0dB (const double fader, double faderCutOff = FaderINFPos)
{
    if (fader < faderCutOff)
      return 0.0;
    // Resembles an analog mixer fader
    double gain = FaderFactorA0dB * exp(FaderFactorB0dB * sqrt( juce::jlimit (0.0, 1.0, fader)));
    return juce::jlimit(0.0, 1.0, gain);
}

/** Maps a gain multiplier 0.0 (-INF) ... 1.0 (0dB) to linear fader/knob position (0..1) */
static double convertGainToFader0dB (const double gain, double faderCutOff = FaderINFPos)
{
    // Inverse function to the above
    double fader =
    juce::jlimit (0.0,
                  1.0,
                  pow(log(juce::jlimit(0.0, 1.0, gain) / FaderFactorA0dB) / FaderFactorB0dB,
                      2.0));
    if (fader < faderCutOff)
      return 0.0;
    return fader;
}


#endif // _TF_EXTENSIONS_H_
