# Sprike

## Virtual Analog Synthesizer

Sprike is an extended version of Tunefish4, originally created by [Brain Control](http://tunefish-synth.com) ([https://github.com/paynebc/tunefish](https://github.com/paynebc/tunefish)), with modifications and enhancements added by Cognitone. While the actual sound synthesis was not changed (in fact, you can import your existing Tunefish4 presets), Sprike features a differently organized preset storage, 4 additional MOD sources, an on-screen keyboard, a level meter, tempo-controlled delay units, and a more complete MIDI implementation with bank plus program selection, in order to integrate more transparently with MIDI-controlled environments and conventions. 

We chose a new name and ID for Sprike, so it can coexist with Tunefish installations out there (versions 4, 3 and 2). In addition, we changed the look and feel, so users won't confuse it with Tunefish4.

IMPORTANT: If you want to fork this project, please change the plugin name, bundle identifiers and manufacturer ID, to make sure it won't clash with installations already deployed out there! That is, replace all traces of Cognitone and Sprike. Thanks. If you want to report bugs and/or suggest enhancements, feel free to contact us at any time.

## Screenshot

![Screenshot](https://github.com/cognitone/sprike/blob/master/media/screenshot.jpg)

## Changes And Enhancements

* Added on-screen keyboard
* Added an output level meter for balancing of preset levels
* Added controls for master output volume, pan
* Added 4 more MOD sources LFO1 INVERSE, LFO2 INVERSE, ADSR1 INVERSE, ADSR2 INVERSE
* Organized programs in (virtual) banks per 128 sounds (folders on disk)
* Express delay times as note lengths (1/4, 1/16, etc) that follow tempo changes
* Implemented MIDI CC volume, pan, reset CC, tempo change
* Implemented MIDI program change message w/bank selection CC0/CC32
* Implemented CC 35 as an option for using sliders for in-track patch selection
* Copy/Paste buttons copy all program parameters to a clipboard and back
* Restore button re-loads program from disk
* Presets stored in compliance with OS conventions (see below)
* Preset count limited to 1024 (8 banks per 128 patches)
* Ensured backward compatibility with tf4 (loads existing presets)
* Visually disable effects and filter units, when not used
* About box giving credit to the creator & contributors
* Minor design tweaks, so users don't confuse this fork with Tunefish4


## Bugs Fixed

* Refactored synth program class with Juce objects (crashes fixed)
* Protected write access to parameters from GUI with Atomics
* Fixed dead LFO sync buttons (had no effect)
* Fixed a generator modulation overflow issue
* Copy/Paste/Restore preset buttons did not work as expected
* Misc minor fixes, speed improvements and code streamlining

## Sounds

* New bank of factory sounds

## Preset Storage

In order to comply with operating system conventions, and to separate user-created presets from factory presets, all presets are now stored in the following folders:

### Mac
`/Library/Audio/Presets/Cognitone/Sprike  (factory)`      
`~/Library/Audio/Presets/Cognitone/Sprike  (user)`      

### Windows 10
`C:\ProgramData\Cognitone\Sprike (factory)`      
`C:\Users\<Username>\AppData\Local\Cognitone\Sprike (user)`      

If you edit and save a factory preset, it will not be overwritten, but stored in your user folder instead. This protects factory presets from accidential modification. For how to import tf4 presets, please read the README file in the factory presets folder.

## How To Compile

Get [Juce](https://www.juce.com) version 5 or later and use Projucer to create project exporters for Windows or Mac. Please refer to the original [Tunefish4](https://github.com/paynebc/tunefish) project for instructions on how to compile for Linux. Possibly Projucer will take care of that already.

## Download

This repository includes source code only. You can download ready-to-use compiled plug-ins with installers for Windows and Mac (VST, VST3 and AudioUnits) from the Cognitone site: [http://www.cognitone.com/get/sprike](http://www.cognitone.com/get/sprike).

## Support The Author

Thank Brain Control for this wonderful synth. Anyone who wants to support this synth, please make a donation to the original author at [tunefish-synth.com](http://tunefish-synth.com). Improvements may eventually also be ported back to Sprike. 
