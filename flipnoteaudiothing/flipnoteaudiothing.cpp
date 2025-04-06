#include <array>
#include <algorithm>
#include <cstdio>
#include <deque>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <random>
#include "adpcm_encoder.cpp"
#define DR_WAV_IMPLEMENTATION
#include "../include/dr_wav.h"

std::string NameGen(std::ifstream& file) {
    char str1e[3];
    file.seekg(0x78);
    file.read(str1e, 3);
    std::stringstream hexStream;
    for (int i = 0; i < 3; i++) {
        hexStream << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (0xFF & (unsigned int)str1e[i]);
    }
    std::string str1 = hexStream.str();
    str1.erase(0, 1); // Remove the first character of str1

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 35);
    std::string randomLetter(1, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[dist(gen)]);
    std::cout << std::format("Your random lucky first character is '{}' (unsure of exact formula rn but rename to OG if you want correct)", randomLetter) << std::endl;

    std::string str2(13, '\0');
    file.seekg(0x7B);
    file.read(&str2[0], 13);

    uint16_t str3e;
    file.seekg(0x88);
    file.read(reinterpret_cast<char*>(&str3e), sizeof(str3e));
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << str3e;
    std::string str3 = ss.str();

    return std::format("{}{}_{}_{}", randomLetter, str1, str2, str3);
}

static bool isNumber(const std::string& str) {
    // Helper function to check if a string is a valid integer
    for (char c : str) {
        if (!isdigit(c)) return false;
    }
    return true;
}

static void moveCursor(int row, int col) {
    // ANSI escape sequence to move cursor to a specific row and column (with colour blue)
    std::cout << "\u001b[34m\033[" << row << ";" << col << "H";
}

int main(int argc, char* argv[]) {
    std::vector<char> bgm;

    const std::string logo[] = {
    "               ..::~:~:~.: . .               ",
    "           .^YY#5#J#55YJY7Y77!~!::.          ",
    "        .~PG5~7?#?B7B~B^G:5:5.Y:! ^.~        ",
    "       ?B#&G#G?~::57G!5!Y~J~?.! :.:^.:..     ",
    "     !G&#&?. .~G?P.^:7Y!J^? :.~.~..  .....   ",
    "   .G&&#&B      .~5^?:Y^P.!.~.^       . :    ",
    "  :&&@B&GB          : 7:Y.            .. .   ",
    " .&&@#&B&GJ           :~7.            : . .  ",
    " B&@#&G&G#P5~.      .!!!!:~        ..: : .   ",
    ".@#@B&57 . ^:7~Y~?!??~J:Y.!^:! . :.        . ",
    "!&###B.         .:.:^~^:^.~.                 ",
    "7&###P                                       ",
    "^&B&PB                                       ",
    " BG@J&~                                      ",
    " :#P&P#                                      ",
    "  !BP#5Y                                     ",
    "   ^PYGY~                                    ",
    "    .??PP~                                   ",
    "      :?YYJ.                                 ",
    "        .~7?Y7..                  .          ",
    "           .^~^?:! .                         ",
    "               . : : : . .                   "
    };

    std::cout << "Hello!!" << std::endl;

    if (argc < 2) {
        std::cout << "Usage: Drag and drop a .wav file onto the tool, or provide its path as an argument" << std::endl;
        system("pause");
        return 1;
    }

    std::string filePath = argv[1];

    std::string path_string = std::filesystem::current_path().string();
    std::cout << path_string << std::endl;
    std::cout << filePath << std::endl;

    system(std::format("ffmpeg -i \"{}\" -ac 1 -y -ar 8192 audio.wav", filePath).c_str());

    std::cout << "Loading BGM..." << std::endl;
    std::string bgm_path = "audio.wav";
    unsigned int channels;
    unsigned int sample_rate; // won't actually be used, but there's not much i can do about that
    uint64_t pcm_frame_count;
    int16_t* sample_data = drwav_open_file_and_read_pcm_frames_s16(bgm_path.c_str(), &channels, &sample_rate, &pcm_frame_count);
    AdpcmEncoder encoder;
    if (!sample_data)
        throw "Could not parse audio.wav!";
    if (channels != 1)
        throw "WAV file needs to be mono!";
    if (pcm_frame_count > UINT32_MAX)
        throw "BGM is too big!";

    // Apply fade in to the beginning (1.0f = 1s = 8192 samples)
    //ApplyFadeIn(sample_data, pcm_frame_count, 0.2f, sample_rate);
    // commented out because it does not work as desired right now

    for (uint64_t i = 0; i < pcm_frame_count; i += 2) {
        bgm.push_back(encoder.EncodeSample(sample_data[i]) | encoder.EncodeSample(sample_data[i + 1]) << 4);
    }

    std::ofstream outFile("output.adpcm", std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(bgm.data()), bgm.size());
    outFile.close();
    std::cout << "File saved as output.adpcm" << std::endl;

    std::ifstream file;
    std::cout << "\n\nEnter flipnote.ppm to insert music to below\nOptional: Add 'import speed' and 'target speed' values! Example:\n'flip.ppm 5 6' (the song speed AND Hz quality will be doubled to suit the 6->12 fps jump)\nMake sure to adjust the song speed manually in Audacity for this. Adjustment guide: https://gbatemp.net/threads/flipnote-nds-ppm-file-direct-audio-import-tool.669125/\n\nYour input: ";

    // FIRE LOGO PRINT
    for (int i = 0; i < 22; i++) {
        // Count the leading spaces
        int leadingSpaces = 0;
        while (leadingSpaces < logo[i].size() && logo[i][leadingSpaces] == ' ') {
            leadingSpaces++;
        }

        // Remove the leading spaces from the line
        std::string lineWithoutSpaces = logo[i].substr(leadingSpaces);

        moveCursor(2 + i, 70 + leadingSpaces);
        std::cout << lineWithoutSpaces;
    }
    std::cout << "\u001b[37m\033[999;13H";
    //end of fire logo print

    std::string fileargs;
    std::getline(std::cin, fileargs);  // Read the entire line of input

    // To split the file args into components
    std::istringstream ss(fileargs);

    std::string filename;
    ss >> std::quoted(filename);
    int importval = 0, targetval = 0;
    if (!(ss >> importval) || importval < 1 || importval > 8) {
        importval = 0;  // If extraction fails or value is out of range, set to 0
    } else if (!(ss >> targetval) || targetval < 1 || targetval > 8) {
        targetval = 0;
        importval = 0;
    }
    if (targetval < importval) {
        std::cout << "Target speed cannot be smaller than import speed (well it can but thats stupid). Keeping default values.\n";
        targetval = 0;
        importval = 0;
    }

    std::cout << filename << std::endl;
    if (importval != 0) {
        std::cout << "Import value: " << importval << std::endl;
        std::cout << "Target value: " << targetval << std::endl;
    }

    file.open(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Couldn't find file :(" << std::endl;
        system("pause");
        return 1;
    }

    uint32_t animSize;
    file.seekg(0x4, std::ios::beg);
    file.read(reinterpret_cast<char*>(&animSize), sizeof(animSize));
    std::cout << "Anim length: " << animSize << std::endl;

    uint16_t frameCount;
    file.seekg(0xC, std::ios::beg);
    file.read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));
    frameCount += 1;
    std::cout << "Frame count: " << frameCount << std::endl;

    int calculatedOffset = 1696 + animSize + frameCount;  // 1696 + 1000 + 50 = 2746
    int remainder = calculatedOffset % 4;     // 2746 % 4 = 2
    int bgmSizeOffset = (remainder == 0) ? calculatedOffset : calculatedOffset + (4 - remainder);  // 2746 + (4 - 2) = 2748

    std::cout << std::format("BGM offset at: {} ({:#x})\n", bgmSizeOffset, bgmSizeOffset);

    uint32_t bgmLength;
    file.seekg(bgmSizeOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&bgmLength), sizeof(bgmLength));
    std::cout << std::format("BGM length is: {}\n", bgmLength);

    file.seekg(0, std::ios::beg); // necessary to go to 0 to read file from 0 to end
    std::vector<char> fileData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());


    *reinterpret_cast<uint32_t*>(&fileData[bgmSizeOffset]) = bgm.size(); // updates bgm size data

    if (targetval > 0) {
        *reinterpret_cast<uint8_t*>(&fileData[bgmSizeOffset + 16]) = 8 - targetval;
        *reinterpret_cast<uint8_t*>(&fileData[bgmSizeOffset + 17]) = 8 - importval;
        std::cout << std::format("Updated import/target speed flags\n");
    }

    int dataSizeStart = bgmSizeOffset + 32; // Start of the chunk to drop
    int dataSizeEnd = dataSizeStart + bgmLength; // End of the chunk to drop

    // Extract the parts we want to keep
    std::vector<char> firstPart(fileData.begin(), fileData.begin() + dataSizeStart);

    std::vector<char> secondPart(fileData.begin() + dataSizeEnd, fileData.end());

    std::vector<char> newData(bgm.begin(), bgm.end());

    // Concatenate firstpart with newdata (music), then secondpart (everything after music except sig, so just sfx if any)
    firstPart.insert(firstPart.end(), newData.begin(), newData.end());
    firstPart.insert(firstPart.end(), secondPart.begin(), secondPart.end() - 0x90);


    std::string newFileName = NameGen(file);
    std::string filePathOutput = newFileName + ".ppm";
    std::ofstream outyFile(filePathOutput, std::ios::binary | std::ios::trunc);
    outyFile.write(firstPart.data(), firstPart.size());

    outyFile.close();

    // totally doesnt just grab the actual rsa key dont be silly
    system("echo -----BEGIN RSA PRIVATE KEY----- > temp.e");
    system("curl -s https://web.archive.org/web/20240819133647/https://www.lampwrights.com/showthread.php?t=28 | awk '/MIICX/{flag=1} flag; /dT7M/{exit}' >> temp.e");
    system("echo -----END RSA PRIVATE KEY----- >> temp.e");

    std::string command = "openssl dgst -sha1 -sign temp.e -out sha1.sign " + filePathOutput;
    system(command.c_str());
    remove("temp.e");

    std::ifstream sigFile("sha1.sign", std::ios::binary);
    std::vector<char> sha1Data((std::istreambuf_iterator<char>(sigFile)), std::istreambuf_iterator<char>());
    sigFile.close();
    sha1Data.resize(sha1Data.size() + 16, '\0');

    firstPart.insert(firstPart.end(), sha1Data.begin(), sha1Data.end());

    outyFile.open(filePathOutput, std::ios::binary | std::ios::trunc); // close n reopen so writing again doesnt append. Opening wipes file.
    outyFile.write(firstPart.data(), firstPart.size());
    remove("sha1.sign");
    
    file.close();
    outyFile.close();

    std::cout << "File successfully updated: " << newFileName << ".ppm" << std::endl;
    system("pause");
    return 0;
}