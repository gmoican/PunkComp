
# PunkComp - A compressor pedal plugin

<img src="demo.jpg" width="70%">

## Introduction
This is a simple vst3/au audio compressor plugin made with [JUCE](https://juce.com/) that I build to introduce myself in the JUCE framework. The functionality is inspired by the Koji Comp by Suhr, although it doesn't sound the same (obviously). The project was mostly used to further my understanding and knowledge of digital signal processing and digital audio effects.
I intend to do a demostration soon enough in my [YouTube channel](https://www.youtube.com/channel/UCe3BPM0SbPYuTHAOrixmqsw).

## Features
- Input Gain (from -18 up to 18 dB).
- Output Gain (from -18 up to 18 dB).
- Compressor dial: It adjust the threshold from 0 to -40dB.
- Attack time (from 1 up to 100ms).
- Mix between dry and wet signal.
- Voice switch: This switch offers the following three voicings.
    - Left: Offers a boost to the upper midrange frequencies to bring out the attack in your picking.
    - Middle: Transparent (flat) frequency response.
    - Right: Offers a boost in the upper midrange and treble frequencies – for a smooth, glassy, top-end sweetness.
- Gain reduction metering.
 
 ## Things I want to improve
 - The compressor implementation uses the JUCE built-in Compressor class. I want to program my own Compressor class to better imitate the behaviour of the Koji Comp.
 - The voicings frequencies and gain factors should be fine tuned.
