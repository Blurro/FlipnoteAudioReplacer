# FlipnoteAudioReplacer
Tool that requires just your .wav audio file and your original flipnote, it replaces the audio and a new .ppm is ready for your DS.
I'm just a huge noob who got really dead set on getting HQ audio into my flipnote and I got myself real deep in this rabbit hole, hope it helps someone else.

**It is against Sudomemo rules to modify flipnotes outside of the app**, buut you might get away with it buuut you might be banned, just a heads up o_o

## Credits

- **Thanks to [Khang06](https://github.com/khang06)** for their original audio encoding code that I found to be a very helpful starting point for this project.
  
- **Thanks to [JoshuaDoes](https://github.com/JoshuaDoes)** for their Go code, which was very helpful for programmatically finding the sounds offset.

- **Thanks to [James](https://github.com/jaames)** for providing useful info about Flipnote audio, including the app's own decompiled functions for encoding and decoding which carry this tool

## Usage

*Encoding+importing* Just drag and drop your .wav file (under 1 min, mp4 works too) onto the tool, and enter your flipnote file when asked!

*Decoding/extracting* Just double click the tool with no arguments, and enter your flipnote file or encoded audio when asked!

Enter your flipnote.ppm file name when prompted (.ppm inclduded), the tool will automatically rename the output flipnote to a name your DS will accept.
*(apparently the first letter uses some odd math including your mac or id ?? I don't believe its been completely figured out yet, just save it again if you want it accurate)*

Full guide with useful info around animation speeds, and how it can help you **double the quality of your song** [found here on GBATemp](https://gbatemp.net/threads/flipnote-nds-ppm-file-direct-audio-import-tool.669125/)
