#include <algorithm>
#undef NDEBUG  // keep assertions in release builds.
#include <cassert>
#include <exception>
#include <execution>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <filesystem>

#include <OpenEXR/IlmImf/ImfArray.h>
#include <OpenEXR/IlmImf/ImfChannelList.h>
#include <OpenEXR/IlmImf/ImfOutputFile.h>
#include <OpenEXR/IlmImf/ImfInputFile.h>
#include <OpenEXR/IlmImf/ImfNamespace.h>

#include "parser.h"
#include "stringutils.h"

#define CALCRESULT(in1, nodeType, in2) do { \
                                        switch (nodeType) { \
                                          case Parser::Node::ADD: \
                                            for (int y = 0; y < res.array.height(); y++) { \
                                              for (int x = 0; x < res.array.width(); x++) { \
                                                res.array[y][x] = (in1) + (in2); \
                                              } \
                                            } \
                                            break; \
                                          case Parser::Node::SUB: \
                                            for (int y = 0; y < res.array.height(); y++) { \
                                              for (int x = 0; x < res.array.width(); x++) { \
                                                res.array[y][x] = (in1) - (in2); \
                                              } \
                                            } \
                                            break; \
                                          case Parser::Node::MULT: \
                                            for (int y = 0; y < res.array.height(); y++) { \
                                              for (int x = 0; x < res.array.width(); x++) { \
                                                res.array[y][x] = (in1) * (in2); \
                                              } \
                                            } \
                                            break; \
                                          case Parser::Node::DIV: \
                                            for (int y = 0; y < res.array.height(); y++) { \
                                              for (int x = 0; x < res.array.width(); x++) { \
                                                res.array[y][x] = (in1) / (in2); \
                                              } \
                                            } \
                                            break; \
                                          default: \
                                            assert(false); \
                                        } \
                                       } while(0)  // Swallowing Semicolon


using namespace std;
using namespace std::placeholders;
namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;

struct CalcResult {
    enum CalcResultType {INVALID, ARRAY, CONSTANT};
    CalcResult() : type(INVALID) {}
    CalcResultType type;
    Array2D<float> array;
    bool hasAlpha;
    float constant;
};

void displayHelp() {
    cout << "Use to compose multiple exr files.\n";
    cout << "examples:\n\n";
    cout << "to simply add two exr files and store it in a new output.exr, do:\n";
    cout << "OpenExrComposer.exe \"outputfile.exr = inputfileA.exr + inputfileB.exr\"\n\n";
    cout << "any math expression consisting of + - * / and () is possible. For example: \n";
    cout << "OpenExrComposer.exe \"beauty.exr = (diffuse.exr * (lighting_raw.exr + gi_raw.exr)) + (reflection_raw.exr * reflection_filter.exr) + (refraction_raw.exr * refraction_filter.exr) + specular.exr + sss.exr + self_illum.exr + caustics.exr + background.exr + atmospheric_effects.exr\"\n\n";
    cout << "You can also compose sequences by using # or ??? as wildcard.\n";
    cout << "A # wildcard can have arbitrary length, while ????? wildcards have the number of characters as questionmarks are used.\n";
    cout << "Example:\n";
    cout << "OpenExrComposer.exe \"beauty_without_reflection_#.exr = beauty_#.exr - reflection#.exr - specular#.exr\"\n\n";
    cout << "You can also use constants. Example:\n";
    cout << "OpenExrComposer.exe \"signed_normals.exr = (unsigned_normals.exr - 0.5) * 2.0\"\n";
    cout << "\n";
    cout << "Compression options:\n";
    cout << "By default, 16 line ZIP compression is used to store the output.\n";
    cout << "To use a different compression, use -c argument or --compression argument to specify the compression.\n";
    cout << "Valid arguments:\n";
    cout << "NO          : uncompressed output\n";
    cout << "RLE         : run length encoding\n";
    cout << "ZIP_SINGLE  : zlib compression, one scan line at a time\n";
    cout << "ZIP         : zlib compression, in blocks of 16 scan lines (the default)\n";
    cout << "PIZ         : piz-based wavelet compression\n";
    cout << "PXR24       : lossy 24-bit float compression\n";
    cout << "B44         : lossy 4-by-4 pixel block compression, fixed compression rate\n";
    cout << "B44A        : lossy 4-by-4 pixel block compression, flat fields are compressed more\n";
    cout << "DWAA        : lossy DCT based compression, in blocks of 32 scanlines. More efficient for partial buffer access\n";
    cout << "DWAB        : lossy DCT based compression, in blocks of 256 scanlines. More efficient space wise and faster to decode full frames than DWAA. (recommended for minimal file size)\n";
    cout << "\n";
    cout << "Alpha options:\n";
    cout << "By default, if one of the input files has an alpha channel, then all input files must have an alpha channel\n";
    cout << "and the output will have an alpha channel too. Use -rgb or --rgb argument to ignore alpha channels.\n\n";
    cout << "Verification:\n";
    cout << "Add the -v or --verify argument to verify that all output files have been written and are valid exr files.";
}

void
writeEXR(const char fileName[],
    const float *pixels,
    int width,
    int height,
    bool hasAlpha,
    Compression compression = ZIP_COMPRESSION)
{

    Header header(width, height);
    header.channels().insert("R", Channel(IMF::FLOAT));
    header.channels().insert("G", Channel(IMF::FLOAT));
    header.channels().insert("B", Channel(IMF::FLOAT));
    if (hasAlpha) {
        header.channels().insert("A", Channel(IMF::FLOAT));
    }

    const int stride = hasAlpha ? 4 : 3;

    header.compression() = compression;

    OutputFile file(fileName, header);

    FrameBuffer frameBuffer;

    frameBuffer.insert("R",                      // name
        Slice(IMF::FLOAT,                        // type
        (char *)pixels,                          // base
        sizeof(*pixels) * stride,                // xStride
        sizeof(*pixels) * stride * width));      // yStride

    frameBuffer.insert("G",                      // name
        Slice(IMF::FLOAT,                        // type
        (char *)(pixels + 1),                    // base
        sizeof(*pixels) * stride,                // xStride
        sizeof(*pixels) * stride * width));      // yStride

    frameBuffer.insert("B",                      // name
        Slice(IMF::FLOAT,                        // type
        (char *)(pixels + 2),                    // base
        sizeof(*pixels) * stride,                // xStride
        sizeof(*pixels) * stride * width));      // yStride

    if (hasAlpha) {
        frameBuffer.insert("A",                  // name
            Slice(IMF::FLOAT,                    // type
            (char *)(pixels + 3),                // base
            sizeof(*pixels) * stride,            // xStride
            sizeof(*pixels) * stride * width));  // yStride
    }

    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}

// Returns true if result is RGBA, false if result is RGB.
bool
readEXR(const char fileName[],
    Array2D<float> &pixels,
    int &width, int &height,
    bool readAlphaIfPresent)
{
    bool readAlpha = false;
    try {
        InputFile file(fileName);

        Header header = file.header();
        const Box2i dw = header.dataWindow();
        width = dw.max.x - dw.min.x + 1;
        height = dw.max.y - dw.min.y + 1;
        const ChannelList channels = header.channels();
        const bool hasAlpha = channels.findChannel("A") != nullptr;
        bool readAlpha = hasAlpha && readAlphaIfPresent;

        const int stride = readAlpha ? 4 : 3;

        pixels.resizeErase(height, width * stride);

        FrameBuffer frameBuffer;

        frameBuffer.insert("R",                            // name
            Slice(IMF::FLOAT,                              // type
                (char *)(&pixels[0][0] -                   // base
                dw.min.x * stride -
                dw.min.y * stride * width),
                sizeof(pixels[0][0]) * stride,             // xStride
                sizeof(pixels[0][0]) * stride * width));   // yStride

        frameBuffer.insert("G",                            // name
            Slice(IMF::FLOAT,                              // type
                (char *)(&pixels[0][1] -                   // base
                dw.min.x * stride -
                dw.min.y * stride * width),
                sizeof(pixels[0][0]) * stride,              // xStride
                sizeof(pixels[0][0]) * stride * width));    // yStride

        frameBuffer.insert("B",                             // name
            Slice(IMF::FLOAT,                               // type
                (char *)(&pixels[0][2] -                    // base
                dw.min.x * stride -
                dw.min.y * stride * width),
                sizeof(pixels[0][0]) * stride,              // xStride
                sizeof(pixels[0][0]) * stride * width));    // yStride

        if (readAlpha) {
            frameBuffer.insert("A",                         // name
                Slice(IMF::FLOAT,                           // type
                    (char *)(&pixels[0][3] -                // base
                    dw.min.x * stride -
                    dw.min.y * stride * width),
                    sizeof(pixels[0][0]) * stride,          // xStride
                    sizeof(pixels[0][0]) * stride * width));// yStride
        }

        file.setFrameBuffer(frameBuffer);
        file.readPixels(dw.min.y, dw.max.y);
    }
    catch (...)
    {
        cout << "Failed to read " << fileName << "\n";
        throw;
    }
    return readAlpha;
}

int main( int argc, char *argv[], char *envp[] ) {

    if(argc < 2) {
        displayHelp();
        return 0;
    }

    vector<string> args(argv + 1, argv + argc);
    string expression = args[0];

    Compression compression = ZIP_COMPRESSION;
    bool readAlpha = true;
    bool verify = false;

    // Loop over remaining command-line args
    for (vector<string>::iterator i = args.begin()+1; i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            displayHelp();
            return 0;
        } else if (*i == "-c" || *i == "--compression") {
            string compressionString = toLower(*++i);
            if (compressionString == "no") {
                compression = NO_COMPRESSION;
            } else if (compressionString == "rle") {
                compression = RLE_COMPRESSION;
            } else if (compressionString == "zip_single") {
                compression = ZIPS_COMPRESSION;
            } else if (compressionString == "zip") {
                compression = ZIP_COMPRESSION;
            } else if (compressionString == "piz") {
                compression = PIZ_COMPRESSION;
            } else if (compressionString == "pxr24") {
                compression = PXR24_COMPRESSION;
            } else if (compressionString == "b44") {
                compression = B44_COMPRESSION;
            } else if (compressionString == "b44a") {
                compression = B44A_COMPRESSION;
            } else if (compressionString == "dwaa") {
                compression = DWAA_COMPRESSION;
            } else if (compressionString == "dwab") {
                compression = DWAB_COMPRESSION;
            } else {
                cout << "unknown compression method: " << *i << "\n";
                displayHelp();
                return 1;
            }
        } else if (*i == "-rgb" || *i == "--rgb") {
            readAlpha = false;
        } else if (*i == "-v" || *i == "--verify") {
            verify = true;
        } else {
            cout << "unknown argument " <<  *i << "\n";
            displayHelp();
            return 1;
        }
    }

    Parser p(expression);
    if(p.isValid()) {
        cout << p.getRoot()->toString() << "\n";
        vector<string> inputFilePaths;
        string outputFilePath;
        std::function<void(const Parser::Node* node)> collectFunc;
        collectFunc = [&](const Parser::Node* node) {
            if (node->type == Parser::Node::INPUTFILEPATH) {
                inputFilePaths.push_back(node->path);
            }
            else if (node->type == Parser::Node::OUTPUTFILEPATH) {
                outputFilePath = node->path;
            }
            if (node->left)
                node->left->evaluate(collectFunc);
            if (node->right)
                node->right->evaluate(collectFunc);
        };
        p.getRoot()->evaluate(collectFunc);
        cout << "Collecting files...\n";
        set<std::string> patches;
        size_t numQuestionMarks = 0;
        for (int i = 0; i < inputFilePaths.size(); i++) {
            string pathString = inputFilePaths[i];
            filesystem::path filePath(pathString);
            filesystem::path folderPath = filePath.parent_path();
            filesystem::path fileName = filePath.filename();
            string fileNameString = fileName.string();
            size_t numHashTags = std::count(fileNameString.begin(), fileNameString.end(), '#');
            size_t countQuestionMarks = std::count(fileNameString.begin(), fileNameString.end(), '?');
            if(numQuestionMarks != 0 && countQuestionMarks != 0 && numQuestionMarks != countQuestionMarks) {
                cout << "error: different number of question marks in input files.\n";
                cout << "use the same amount of question marks for each wildcard definition.\n";
                return 1;
            }
            numQuestionMarks = countQuestionMarks;
            if(numQuestionMarks != 0 && numHashTags != 0) {
                cout << "error: cannot mix # and ? wildcards in same expression. use either one.\n";
                return 1;
            }
            size_t numWildcards = numHashTags;
            size_t searchStartPos = 0;
            while(fileNameString.find('?', searchStartPos) != string::npos) {
              numWildcards++;
              searchStartPos = fileNameString.find_first_not_of('?', searchStartPos);
            }
            if (numWildcards > 1) {
                cout << "error: multiple wildcards in " << filePath.string() << "\n";
                cout << "use only one wildcard per filename.\n";
                return 1;
            }
            else if (numWildcards == 1) {
                vector<string> nameSplit;
                if(numQuestionMarks > 0) {
                  nameSplit = split(toLower(fileNameString), string(numQuestionMarks, '?'));
                } else {
                  assert(numHashTags == 1);
                  nameSplit = split(toLower(fileNameString), "#");
                }
                assert(nameSplit.size() == 2);
                for (const auto & entry : filesystem::directory_iterator(folderPath)) {
                    std::string otherFileName = toLower(entry.path().filename().string());
                    size_t prefixPos = otherFileName.find(nameSplit[0]);
                    size_t postfixPos = otherFileName.find(nameSplit[1]);
                    if (prefixPos == 0 && postfixPos != string::npos) {
                        string patch = otherFileName.substr(nameSplit[0].size(),
                            otherFileName.size() - nameSplit[0].size() - nameSplit[1].size());
                        if(numHashTags == 1 || (patch.length() == numQuestionMarks)) {
                          patches.insert(patch);
                        }
                    }
                }
            }
            else {
                if (!filesystem::exists(filePath)) {
                    cout << "error: " << filePath.string() << " does not exist.\n";
                    return 1;
                }
            }
        }
        //  Check if all necessary input files exist and output error otherwise:
        vector<string> missingFiles;
        vector<string> outputFilePaths;
        for (string patch : patches) {
            // find missing files.
            for (int i = 0; i < inputFilePaths.size(); i++) {
                string pathString = inputFilePaths[i];
                size_t wildCardLength = 1;
                size_t wildCardPos = pathString.find("#");
                if(wildCardPos == string::npos) {
                  wildCardPos = pathString.find(string(numQuestionMarks, '?'));
                  wildCardLength = numQuestionMarks;
                }
                assert(wildCardPos != string::npos);
                pathString.replace(wildCardPos, wildCardLength, patch);
                filesystem::path filePath(pathString);
                if (!filesystem::exists(filePath)) {
                    missingFiles.push_back(pathString);
                }
            }
            // collect output file paths.
            string pathString = outputFilePath;
            size_t wildCardLength = 1;
            size_t wildCardPos = pathString.find("#");
            if(wildCardPos == string::npos) {
                wildCardPos = pathString.find(string(numQuestionMarks, '?'));
                wildCardLength = numQuestionMarks;
            }
            assert(wildCardPos != string::npos);
            pathString.replace(wildCardPos, wildCardLength, patch);
            filesystem::path filePath(pathString);
            outputFilePaths.push_back(pathString);
        }
        if(patches.empty()) {
            outputFilePaths.push_back(outputFilePath);
        }
        if (!missingFiles.empty()) {
            cout << "error: Based on wildcards, the following files would be needed, but they don't exist:\n";
            for (string path : missingFiles) {
                cout << path << "\n";
            }
            return 1;
        }

        std::function<void(const Parser::Node*, const string& patch, CalcResult&)> evaluationFunc = [&](const Parser::Node* node, const string& patch, CalcResult& res) {
            switch (node->type) {
            case Parser::Node::INPUTFILEPATH: {
                    int width, height;
                    res.type = CalcResult::ARRAY;
                    string fileName = node->path;
                    size_t wildCardLength = 1;
                    size_t wildCardPos = fileName.find("#");
                    if(wildCardPos == string::npos) {
                      wildCardPos = fileName.find(string(numQuestionMarks, '?'));
                      wildCardLength = numQuestionMarks;
                    }
                    if(wildCardPos != string::npos)
                        fileName.replace(wildCardPos, wildCardLength, patch);
                    res.hasAlpha = readEXR(fileName.c_str(), res.array, width, height, readAlpha);
                    return;
                }
                case Parser::Node::CONSTANT:
                    res.type = CalcResult::CONSTANT;
                    res.constant = node->constant;
                    return;
                case Parser::Node::ADD:
                case Parser::Node::SUB:
                case Parser::Node::MULT:
                case Parser::Node::DIV:
                {
                    assert(node->left && node->right);
                    CalcResult leftResult, rightResult;
                    std::function<void(const Parser::Node*)> closed = [&](const Parser::Node* node) { evaluationFunc(node, patch, leftResult); };
                    node->left->evaluate(closed);
                    closed = [&](const Parser::Node* node) { evaluationFunc(node, patch, rightResult); };
                    node->right->evaluate(closed);
                    assert(leftResult.type != CalcResult::INVALID);
                    assert(rightResult.type != CalcResult::INVALID);
                    if (leftResult.type == CalcResult::CONSTANT && rightResult.type == CalcResult::CONSTANT) {
                        res.type = CalcResult::CONSTANT;
                        switch(node->type) {
                            case Parser::Node::ADD:
                                res.constant = leftResult.constant + rightResult.constant;
                                break;
                            case Parser::Node::SUB:
                                res.constant = leftResult.constant - rightResult.constant;
                                break;
                            case Parser::Node::MULT:
                                res.constant = leftResult.constant * rightResult.constant;
                                break;
                            case Parser::Node::DIV:
                                res.constant = leftResult.constant / rightResult.constant;
                                break;
                            default:
                                assert(false);
                        }
                    }
                    else if (leftResult.type == CalcResult::ARRAY && rightResult.type == CalcResult::ARRAY) {
                        res.type = CalcResult::ARRAY;
                        res.array.resizeErase(leftResult.array.height(), leftResult.array.width());
                        if (leftResult.array.width() != rightResult.array.width() || leftResult.array.height() != rightResult.array.height()) {
                            cout << "error: resolution mismatch.\n";
                            assert(false);
                        }
                        if (leftResult.hasAlpha != rightResult.hasAlpha) {
                            cout << "error: some inputs have Alpha channels, others do not. Consider using -rgb argument.\n";
                            assert(false);
                        }
                        res.hasAlpha = leftResult.hasAlpha;
                        CALCRESULT(leftResult.array[y][x], node->type, rightResult.array[y][x]);
                    }
                    else if (leftResult.type == CalcResult::ARRAY) {
                        assert(rightResult.type == CalcResult::CONSTANT);
                        // One Array operand and one constant operand.
                        res.type = CalcResult::ARRAY;
                        res.array.resizeErase(leftResult.array.height(), leftResult.array.width());
                        res.hasAlpha = leftResult.hasAlpha;
                        const float c = rightResult.constant;
                        CALCRESULT(leftResult.array[y][x], node->type, c);
                    }
                    else if (rightResult.type == CalcResult::ARRAY) {
                        assert(leftResult.type == CalcResult::CONSTANT);
                        // One Array operand and one constant operand.
                        res.type = CalcResult::ARRAY;
                        res.array.resizeErase(rightResult.array.height(), rightResult.array.width());
                        res.hasAlpha = rightResult.hasAlpha;
                        const float c = leftResult.constant;
                        CALCRESULT(c, node->type, rightResult.array[y][x]);
                    }
                    return;
                }
                default:
                    assert(false);
            }
        };
        const Parser::Node* root = p.getRoot();
        if (patches.empty())
            patches.insert("");
        std::for_each(
            std::execution::par_unseq,
            patches.begin(),
            patches.end(),
            [&](const string& patch)
            {
                string targetFileName = root->left->path;
                size_t wildCardLength = 1;
                size_t wildCardPos = targetFileName.find("#");
                if(wildCardPos == string::npos) {
                    wildCardPos = targetFileName.find(string(numQuestionMarks, '?'));
                    wildCardLength = numQuestionMarks;
                }
                if(wildCardPos != string::npos)
                    targetFileName.replace(wildCardPos, wildCardLength, patch);
                cout << "computing " << targetFileName << "               \r";
                CalcResult res;
                std::function<void(const Parser::Node*)> closed = [&](const Parser::Node* node) { evaluationFunc(node, patch, res); };
                root->right->evaluate(closed);
                assert(res.type == CalcResult::ARRAY);
                const int stride = res.hasAlpha ? 4 : 3;
                writeEXR(targetFileName.c_str(), res.array[0], res.array.width() / stride, res.array.height(), res.hasAlpha, compression);
            });
        if(verify) {
            cout << "verifying written images...";
            Array2D<float> pixels;
            int width, height;
            bool verificationSuccessful = true;
            for(int i=0; i<outputFilePaths.size(); i++) {
                string pathString = outputFilePaths[i];
                filesystem::path filePath(pathString);
                if (!filesystem::exists(filePath)) {
                    cout << "error: " << filePath.string() << " has not been written.\n";
                    verificationSuccessful = false;
                }
                else if(filesystem::file_size(pathString) == 0) {
                    cout << "error: " << filePath.string() << " is 0 bytes.\n";
                    verificationSuccessful = false;
                } else
                {
                    try {
                        readEXR(pathString.c_str(), pixels, width, height, true);
                    }
                    catch(...) {
                        cout << "error: verification failed. " << filePath.string() << " could not be read.\n";
                        verificationSuccessful = false;
                    }
                }
            }
            if(verificationSuccessful) {
                cout << "verification succeeded, " <<  outputFilePaths.size() <<" files have been written.\n";
            } else {
                cout << "verification failed.\n";
            }
        }
    } else {
        cout << "error parsing expression: " << p.getErrorMessage() << "\n";
    }
}