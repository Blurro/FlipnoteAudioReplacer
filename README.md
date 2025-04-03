# FlipnoteAudioReplacer
Tool that requires just your .wav audio file and your original flipnote, it replaces the audio and a new .ppm is ready for your DS.
I'm just a huge noob who got really dead set on getting HQ audio into my flipnote and I got myself real deep in this rabbit hole, hope it helps someone else.

## Credits

- **Thanks to [@Khang06](https://github.com/khang06)** for their audio conversion code (which I take ZERO credit for), I have no idea how it differs from simple ffmpeg and sox commands I just know that only this works.
  
- **Thanks to [JoshuaDoes](https://github.com/JoshuaDoes)** for their Go code, which was very helpful for programmatically finding the sounds offset.

## Usage

Open CMD in the same folder as the tool and just do 'flipaud input.wav' or drag and drop the .wav onto the tool (not the open window, the exe in file explorer!)
Enter your flipnote.ppm file name when prompted (.ppm inclduded), the tool will automatically rename the output flipnote to a name your DS will accept.
