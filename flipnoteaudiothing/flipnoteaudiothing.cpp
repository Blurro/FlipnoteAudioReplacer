#define _CRT_SECURE_NO_WARNINGS
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
#include <cstdlib>
#include <string>
#include <windows.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/applink.c>
#define DR_WAV_IMPLEMENTATION
#include "../include/dr_wav.h"

bool sign_file(const std::string& privkey_file, const std::string& input_file, const std::string& output_sig) {
    FILE* fp = nullptr;
    if (fopen_s(&fp, privkey_file.c_str(), "r") != 0 || !fp) return false;

    EVP_PKEY* pkey = PEM_read_PrivateKey(fp, nullptr, nullptr, nullptr);
    fclose(fp);
    if (!pkey) return false;

    std::ifstream in(input_file, std::ios::binary);
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return false;
    }

    unsigned char sig[256];
    size_t sig_len = sizeof(sig);

    bool success = false;
    if (EVP_SignInit(ctx, EVP_sha1()) &&
        EVP_SignUpdate(ctx, buf.data(), buf.size()) &&   // pass raw data, not hash
        EVP_SignFinal(ctx, sig, (unsigned int*)&sig_len, pkey)) {
        std::ofstream out(output_sig, std::ios::binary);
        out.write((char*)sig, sig_len);
        out.close();
        success = true;
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return success;
}

std::string NameGen(std::unique_ptr<std::istream>& file) {
    char str1e[3];
    file->seekg(0x78);
    file->read(str1e, 3);
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
    file->seekg(0x7B);
    file->read(&str2[0], 13);

    uint16_t str3e;
    file->seekg(0x88);
    file->read(reinterpret_cast<char*>(&str3e), sizeof(str3e));
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << str3e;
    std::string str3 = ss.str();

    return std::format("{}{}_{}_{}", randomLetter, str1, str2, str3);
}

uint16_t frameCount;
int calculateBgmSizeOffset(std::unique_ptr<std::istream>& file) {
    uint32_t animSize;
    file->seekg(0x4, std::ios::beg);
    file->read(reinterpret_cast<char*>(&animSize), sizeof(animSize));
    std::cout << "Anim length: " << animSize << std::endl;

    file->seekg(0xC, std::ios::beg);
    file->read(reinterpret_cast<char*>(&frameCount), sizeof(frameCount));
    frameCount += 1;
    std::cout << "Frame count: " << frameCount << std::endl;

    int calculatedOffset = 1696 + animSize + frameCount;
    int remainder = calculatedOffset % 4;
    int bgmSizeOffset = (remainder == 0) ? calculatedOffset : calculatedOffset + (4 - remainder);

    return bgmSizeOffset;
}

std::istringstream ss;
std::string fileExtension;
std::unique_ptr<std::istream> openFileFromInput() {
    std::string fileargs;
    std::getline(std::cin, fileargs);

    // Replace backslashes with forward slashes before quoted() removes them
    for (char& c : fileargs) {
        if (c == '\\') c = '/';
    }
    ss.str(fileargs);

    std::string filename;
    ss >> std::quoted(filename);

    size_t dotPos = filename.rfind('.');
    if (dotPos != std::string::npos) {
        fileExtension = filename.substr(dotPos);
    }

    auto file = std::make_unique<std::ifstream>(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Couldn't find file :(" << std::endl;
        system("pause");
        file->setstate(std::ios::failbit);
        return file;
    }
    return file;  // Return the open file stream
}

std::string getExeDirectory() {
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        std::filesystem::path exeFilePath(exePath);
        return exeFilePath.parent_path().string(); // Return the directory part
    }
    return "";  // Return an empty string if something went wrong
}

void FireLogoPrint(int x, int y, int endx, int endy) {
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
    for (int i = 0; i < 22; i++) {
        int leadingSpaces = 0;
        while (leadingSpaces < logo[i].size() && logo[i][leadingSpaces] == ' ') {
            leadingSpaces++;
        }
        std::string lineWithoutSpaces = logo[i].substr(leadingSpaces);
        std::cout << "\u001b[34m\033[" << y + i << ";" << x + leadingSpaces << "H";
        std::cout << lineWithoutSpaces;
    }
    std::cout << std::format("\u001b[37m\033[{};{}H", endy, endx); // did it differently to just a few lines ago cus consistency whooo pshhh
}

constexpr double getFpsFromSpeed(int speed) {
    switch (speed) {
    case 1: return 0.5;
    case 2: return 1;
    case 3: return 2;
    case 4: return 4;
    case 5: return 6;
    case 6: return 12;
    case 7: return 20;
    case 8: return 30;
    default: return 0;
    }
}

int main(int argc, char* argv[]) {
    std::vector<char> bgm;

    std::cout << "Hello!!" << std::endl;

    // if we detect regular cmd instead of terminal skip the logo stuff
    bool isCmd = false;
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow != NULL) {
        char className[256];
        GetClassNameA(consoleWindow, className, sizeof(className));
        if (std::string(className).find("ConsoleWindowClass") != std::string::npos) {
            isCmd = true;
        }
    }

    std::string exeDirectory = getExeDirectory();
    if (exeDirectory.empty()) {
        std::cerr << "Failed to determine the executable directory." << std::endl;
        return 1;
    }

    if (argc < 2) {
        std::string promppy = "OR enter a flip.ppm/.adpcm to extract a .wav from: ";
        std::cout << "Usage for import: Drag and drop a .wav file onto the tool, or provide its path as an argument\n\n" << promppy;

        if (!isCmd) {
            FireLogoPrint(56, 5, promppy.length() + 1, 4);
        }

        std::unique_ptr<std::istream> inputFile = openFileFromInput();
        if (!inputFile) { return 1; }

        if (fileExtension == ".ppm") {
            int bgmSizeOffset = calculateBgmSizeOffset(inputFile);
            std::cout << std::format("BGM offset at: {} ({:#x})\n", bgmSizeOffset, bgmSizeOffset);

            uint32_t bgmLength;
            inputFile->seekg(bgmSizeOffset, std::ios::beg);
            inputFile->read(reinterpret_cast<char*>(&bgmLength), sizeof(bgmLength));
            std::cout << std::format("BGM length is: {}\n", bgmLength);

            inputFile->seekg(bgmSizeOffset + 32, std::ios::beg);
            std::vector<char> adpcmData(bgmLength); // makes vector with size 'bgmlength'
            inputFile->read(adpcmData.data(), bgmLength); // reads from current seek (chosen above) until vector is full
            inputFile = std::make_unique<std::istringstream>(std::string(adpcmData.begin(), adpcmData.end())); // sets inputfile to the adpcm data
        }
        else if (fileExtension != ".adpcm") {
            std::cerr << "Can't decode audio from this type (enter a ppm or adpcm file)" << std::endl;
            system("pause");
            return 0;
        }

        uint16_t initialPredict;
        uint8_t initialStep;
        inputFile->read(reinterpret_cast<char*>(&initialPredict), sizeof(initialPredict));
        inputFile->seekg(2, std::ios::beg);
        inputFile->read(reinterpret_cast<char*>(&initialStep), sizeof(initialStep));

        std::cout << "Initial predictor: " << initialPredict << "\n";
        std::cout << "Initial step index: " << static_cast<int>(initialStep) << "\n";

        inputFile->seekg(4, std::ios::beg); // skip first 4 bytes
        std::vector<char> inputData((std::istreambuf_iterator<char>(*inputFile)), std::istreambuf_iterator<char>());

        // Prepare output buffer for decoded samples
        std::vector<char> outputData(inputData.size() * 4);

        AdpcmDecoder decoder;

        // Decode the ADPCM data into outputData
        int numSamples = inputData.size() * 2;
        decoder.DecodeSamples(inputData.data(), outputData.data(), numSamples, initialPredict, initialStep);

        // Open the WAV file for writing
        std::ofstream outputFile("decoded.wav", std::ios::binary);
        if (!outputFile) {
            std::cerr << "Failed to save decoded.wav." << std::endl;
            return 1;
        }

        // Write the WAV header
        writeWavHeader(outputFile, outputData.size());

        // Write the decoded audio samples (16-bit PCM data)
        outputFile.write(outputData.data(), outputData.size());
        outputFile.close();

        std::cout << "Decoding complete. Output saved to 'decoded.wav'." << std::endl;

        system("pause");
        return 0;
    }

    int ffmpegFound = 3;
    std::string ffmpegPath = exeDirectory + "\\ffmpeg.exe";
    if (std::filesystem::exists(ffmpegPath)) {
        ffmpegFound = 1;
        std::cout << "FFmpeg found in executable directory." << std::endl;
    }
    else {
        int result = system("ffmpeg -version >nul 2>&1");
        if (result != 0) {  // Check if ffmpeg is available in the system PATH
            std::cout << "FFmpeg is not installed (not found on PATH) AND no FFmpeg.exe next to tool\nDownloading ffmpeg.exe (to tool directory)..." << std::endl;
            std::string ffmpegDownloadCmd = "curl -L -o \"" + ffmpegPath + "\" https://github.com/Blurro/FlipnoteAudioReplacer/raw/refs/heads/main/ffmpeg.exe";
            system(ffmpegDownloadCmd.c_str());
            std::cout << "Done, try tool again." << std::endl;
            system("pause");
            return 1;
        }
        else {
            ffmpegFound = 2;
            std::cout << "FFmpeg found in system PATH." << std::endl;
        }
    }

    std::string filePath = argv[1];
    //std::string filePath = "ogextract.wav";
    std::cout << filePath << std::endl;

    std::string ext = filePath.substr(filePath.find_last_of('.'));
    if (ext == ".ppm" || ext == ".adpcm") {
        std::cout << "You're supposed to drag an audio file on here for importing! To extract audio instead, double click the tool and enter when prompted\n";
        system("pause");
        return 1;
    }

    if (ffmpegFound == 1) {
        system(std::format("cmd /C \"\"{}\" -i \"{}\" -ac 1 -y -ar 8192 audio.wav\"", ffmpegPath, filePath).c_str());
    }
    else if (ffmpegFound == 2) {
        system(std::format("cmd /C \"ffmpeg -i \"{}\" -ac 1 -y -ar 8192 audio.wav\"", filePath).c_str());
    }
    else {
        return 1;
    }

    std::cout << "Loading BGM..." << std::endl;
    std::string bgm_path = "audio.wav";
    unsigned int channels;
    unsigned int sample_rate;
    uint64_t pcm_frame_count;
    int16_t* sample_data = drwav_open_file_and_read_pcm_frames_s16(bgm_path.c_str(), &channels, &sample_rate, &pcm_frame_count);

    if (!sample_data) throw "Could not parse audio.wav!";
    if (channels != 1) throw "WAV file needs to be mono!";

    AdpcmState initialState = EncodeInitialState(sample_data[0]);
    if (initialState.stepIndex > 26) {
        std::cout << "Initial step was too high ("
            << static_cast<int>(initialState.stepIndex)
            << "), defaulting to 0." << std::endl;
        initialState.stepIndex = 0;
    }
    if (pcm_frame_count > 491520) {
        std::cout << "BGM is longer than 1 minute!" << std::endl;
        system("pause");
        return 1;
    }
    std::cout << "Best initial step index: " << static_cast<int>(initialState.stepIndex) << std::endl;

    AdpcmEncoder encoder;
    encoder.step_index = initialState.stepIndex;
    for (uint64_t i = 0; i < pcm_frame_count; i += 2) {
        bgm.push_back(encoder.EncodeSample(sample_data[i]) | encoder.EncodeSample(sample_data[i + 1]) << 4);
    }
    // add adpcm header
    std::vector<char> header = {
    0x00,
    0x00,
    static_cast<char>(initialState.stepIndex),
    0x00 };
    bgm.insert(bgm.begin(), header.begin(), header.end());

    double seconds = pcm_frame_count / 8192.0;
    seconds = std::round(seconds * 1000.0) / 1000.0;
    std::cout << std::format("Imported song length: {}s\n", seconds);

    std::ofstream outFile("output.adpcm", std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(bgm.data()), bgm.size());
    outFile.close();
    std::cout << "File saved as output.adpcm" << std::endl;

    std::cout << "\n\nEnter flipnote.ppm to insert music to below\nOptional: Add 'import speed' and 'target speed' values! Example:\n'flip.ppm 5 6' (the song speed AND Hz quality will be doubled to suit the 6->12 fps jump)\nMake sure to adjust the song speed manually in Audacity for this. Adjustment guide: https://gbatemp.net/threads/flipnote-nds-ppm-file-direct-audio-import-tool.669125/\n\nYour input: ";

    // FIRE LOGO PRINT
    if (!isCmd) {
        FireLogoPrint(70, 2, 13, 999);
    }
    //end of fire logo print

    std::unique_ptr<std::istream> file = openFileFromInput();
    if (!file) {
        return 1;
    }

    int importval = 0, targetval = 0;
    if (!(ss >> importval) || importval < 1 || importval > 8) {
        importval = 0;  // If extraction fails or value is out of range, set to 0
    }
    else if (!(ss >> targetval) || targetval < 1 || targetval > 8) {
        targetval = 0;
        importval = 0;
    }
    if (targetval < importval) {
        std::cout << "Target speed cannot be smaller than import speed (well it can but thats stupid). Keeping default values.\n";
        targetval = 0;
        importval = 0;
    }

    int bgmSizeOffset = calculateBgmSizeOffset(file);

    std::cout << std::format("BGM offset at: {} ({:#x})\n", bgmSizeOffset, bgmSizeOffset);

    uint32_t bgmLength;
    file->seekg(bgmSizeOffset, std::ios::beg);
    file->read(reinterpret_cast<char*>(&bgmLength), sizeof(bgmLength));
    std::cout << std::format("BGM length is: {}\n", bgmLength);

    file->seekg(0, std::ios::beg); // necessary to go to 0 to read file from 0 to end
    std::vector<char> fileData((std::istreambuf_iterator<char>(*file)), std::istreambuf_iterator<char>());

    *reinterpret_cast<uint32_t*>(&fileData[bgmSizeOffset]) = bgm.size(); // updates bgm size data

    if (targetval > 0) {
        *reinterpret_cast<uint8_t*>(&fileData[bgmSizeOffset + 16]) = 8 - targetval;
        *reinterpret_cast<uint8_t*>(&fileData[bgmSizeOffset + 17]) = 8 - importval;
        std::cout << std::format("Updated import/target speed flags\n");
    }
    else {
        targetval = 8 - static_cast<int8_t>(fileData[bgmSizeOffset + 16]);
        importval = 8 - static_cast<int8_t>(fileData[bgmSizeOffset + 17]);
    }
    std::cout << "Import value: " << importval << std::endl;
    std::cout << "Target value: " << targetval << std::endl;

    double fpsImport = getFpsFromSpeed(importval);
    double fpsTarget = getFpsFromSpeed(targetval);
    double multip = fpsImport / fpsTarget;
    double newseconds = seconds * multip;
    double framecalc = newseconds * fpsTarget;
    multip = std::round(multip * 1000.0) / 1000.0;
    newseconds = std::round(newseconds * 1000.0) / 1000.0;
    framecalc = std::round(framecalc * 1000.0) / 1000.0;
    if (importval != targetval) {
        std::cout << std::format("Speed {}->{} ({}FPS to {}FPS) multiplier: {}\n", importval, targetval, fpsImport, fpsTarget, multip);
    }
    std::cout << std::format("New song duration is {}s and this covers {} frames of speed {}! ({} frames in flip)\n", newseconds, framecalc, targetval, frameCount);

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

    if (!sign_file("temp.e", filePathOutput, "sha1.sign")) {
        std::cerr << "signing failed, manually use the key in temp.e to generate a sig over the current output ppm, append the result sig to the end of the ppm, followed by 16 null bytes. your ppm should now be 144 bytes bigger (skipping this will cause flipnote to delete it)\n";
        system("pause");
        return 1;
    }
    remove("temp.e");

    std::ifstream sigFile("sha1.sign", std::ios::binary);
    std::vector<char> sha1Data((std::istreambuf_iterator<char>(sigFile)), std::istreambuf_iterator<char>());
    sigFile.close();
    sha1Data.resize(sha1Data.size() + 16, '\0');

    firstPart.insert(firstPart.end(), sha1Data.begin(), sha1Data.end());

    outyFile.open(filePathOutput, std::ios::binary | std::ios::trunc); // close n reopen so writing again doesnt append. Opening wipes file.
    outyFile.write(firstPart.data(), firstPart.size());
    remove("sha1.sign");

    outyFile.close();

    std::cout << "File successfully updated: " << newFileName << ".ppm" << std::endl;
    system("pause");
    return 0;
}