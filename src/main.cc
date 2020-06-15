#include <algorithm>
#include <cassert>
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

#define CALCRESULT(in1, op, in2) for (int y = 0; y < res.array.height(); y++) { \
                                   for (int x = 0; x < res.array.width(); x++) { \
                                     res.array[y][x] = (in1) op (in2); \
                                   } \
                                 }

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
    float constant;
};

void displayHelp() {
    cout << "Use to compose multiple exr files.\n";
    cout << "examples:\n\n";
    cout << "to simply add two exr files and store it in a new output.exr, do:\n";
    cout << "OpenExrComposer.exe \"outputfile.exr = inputfileA.exr + inputfileB.exr\"\n\n";
    cout << "any math expression consisting of + - * / and () is possible. For example: \n";
    cout << "OpenExrComposer.exe \"beauty.exr = (diffuse.exr * (lighting_raw.exr + gi_raw.exr)) + (reflection_raw.exr * reflection_filter.exr) + (refraction_raw.exr * refraction_filter.exr) + specular.exr + sss.exr + self_illum.exr + caustics.exr + background.exr + atmospheric_effects.exr\"\n\n";
    cout << "You can also compose sequences by using # as wildcard character. Example: \n";
    cout << "OpenExrComposer.exe \"beauty_without_reflection_#.exr = beauty_#.exr - reflection#.exr - specular#.exr\"\n\n";
    cout << "You can also use constants. Example:\n";
    cout << "OpenExrComposer.exe \"signed_normals.exr = (unsigned_normals.exr - 0.5) * 2.0\"\n";
    cout << "\n";
    cout << "Compression options:\n";
    cout << "By default, 16 line ZIP compression is used to store the output.\n";
    cout << "To use a different compression, use -c argument or -compression argument to specify the compression.\n";
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
}

void
writeRGB(const char fileName[],
    const float *rgbPixels,
    int width,
    int height,
    Compression compression = ZIP_COMPRESSION)
{

    Header header(width, height);
    header.channels().insert("R", Channel(IMF::FLOAT));
    header.channels().insert("G", Channel(IMF::FLOAT));
    header.channels().insert("B", Channel(IMF::FLOAT));

    header.compression() = compression;

    OutputFile file(fileName, header);

    FrameBuffer frameBuffer;

    frameBuffer.insert("R",					// name
        Slice(IMF::FLOAT,					// type
        (char *)rgbPixels,					// base
        sizeof(*rgbPixels) * 3,				// xStride
        sizeof(*rgbPixels) * 3 * width));	// yStride

    frameBuffer.insert("G",					// name
        Slice(IMF::FLOAT,					// type
        (char *)(rgbPixels+1),				// base
        sizeof(*rgbPixels) * 3,				// xStride
        sizeof(*rgbPixels) * 3 * width));	// yStride

    frameBuffer.insert("B",					// name
        Slice(IMF::FLOAT,					// type
        (char *)(rgbPixels + 2),			// base
        sizeof(*rgbPixels) * 3,				// xStride
        sizeof(*rgbPixels) * 3 * width));	// yStride

    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}

void
readRGB(const char fileName[],
    Array2D<float> &rgbPixels,
    int &width, int &height)
{
    InputFile file(fileName);

    Header header = file.header();
    Box2i dw = header.dataWindow();
    width = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;

    rgbPixels.resizeErase(height, width*3);

    FrameBuffer frameBuffer;

    frameBuffer.insert("R",							// name
        Slice(IMF::FLOAT,							// type
            (char *)(&rgbPixels[0][0] -				// base
            dw.min.x * 3 -
            dw.min.y * 3 * width),
            sizeof(rgbPixels[0][0]) * 3,			// xStride
            sizeof(rgbPixels[0][0]) * 3 * width));	// yStride

    frameBuffer.insert("G",							// name
        Slice(IMF::FLOAT,							// type
            (char *)(&rgbPixels[0][1] -				// base
            dw.min.x * 3 -
            dw.min.y * 3 * width),
            sizeof(rgbPixels[0][0]) * 3,			// xStride
            sizeof(rgbPixels[0][0]) * 3 * width));	// yStride

    frameBuffer.insert("B",							// name
        Slice(IMF::FLOAT,							// type
            (char *)(&rgbPixels[0][2] -				// base
            dw.min.x * 3 -
            dw.min.y * 3 * width),
            sizeof(rgbPixels[0][0]) * 3,			// xStride
            sizeof(rgbPixels[0][0]) * 3 * width));	// yStride

    file.setFrameBuffer(frameBuffer);
    file.readPixels(dw.min.y, dw.max.y);
}

int main( int argc, char *argv[], char *envp[] ) {

    if(argc < 2) {
        displayHelp();
        return 0;
    }

    vector<string> args(argv + 1, argv + argc);
    string expression = args[0];

    Compression compression = ZIP_COMPRESSION;

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
        for (int i = 0; i < inputFilePaths.size(); i++) {
            string pathString = inputFilePaths[i];
            filesystem::path filePath(pathString);
            filesystem::path folderPath = filePath.parent_path();
            filesystem::path fileName = filePath.filename();
            string fileNameString = fileName.string();
            size_t numWildcards = std::count(fileNameString.begin(), fileNameString.end(), '#');
            if (numWildcards > 1) {
                cout << "error: multiple # in " << filePath.string() << "\n";
                cout << "use only one # wildcard per filename.\n";
                return 1;
            }
            else if (numWildcards == 1) {
                vector<string> nameSplit = split(toLower(fileNameString), "#");
                assert(nameSplit.size() == 2);
                for (const auto & entry : filesystem::directory_iterator(folderPath)) {
                    std::string otherFileName = toLower(entry.path().filename().string());
                    size_t prefixPos = otherFileName.find(nameSplit[0]);
                    size_t postfixPos = otherFileName.find(nameSplit[1]);
                    if (prefixPos == 0 && postfixPos != string::npos) {
                        string patch = otherFileName.substr(nameSplit[0].size(),
                            otherFileName.size() - nameSplit[0].size() - nameSplit[1].size());
                        patches.insert(patch);
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
        for (string patch : patches) {
            for (int i = 0; i < inputFilePaths.size(); i++) {
                string pathString = inputFilePaths[i];
                size_t hashPos = pathString.find("#");
                assert(hashPos != string::npos);
                pathString.replace(hashPos, 1, patch);
                filesystem::path filePath(pathString);
                if (!filesystem::exists(filePath)) {
                    missingFiles.push_back(pathString);
                }
            }
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
                    size_t hashPos = fileName.find("#");
                    if(hashPos != string::npos)
                        fileName.replace(hashPos, 1, patch);
                    readRGB(fileName.c_str(), res.array, width, height);
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
                        res.constant = leftResult.constant * rightResult.constant;
                    }
                    else if (leftResult.type == CalcResult::ARRAY && rightResult.type == CalcResult::ARRAY) {
                        res.type = CalcResult::ARRAY;
                        res.array.resizeErase(leftResult.array.height(), leftResult.array.width());
                        assert(leftResult.array.width() == rightResult.array.width() && leftResult.array.height() == rightResult.array.height());
                        switch (node->type) {
                        case Parser::Node::ADD:
                            CALCRESULT(leftResult.array[y][x], +, rightResult.array[y][x])
                            break;
                        case Parser::Node::SUB:
                            CALCRESULT(leftResult.array[y][x], -, rightResult.array[y][x])
                            break;
                        case Parser::Node::MULT:
                            CALCRESULT(leftResult.array[y][x], *, rightResult.array[y][x])
                            break;
                        case Parser::Node::DIV:
                            CALCRESULT(leftResult.array[y][x], /, rightResult.array[y][x])
                            break;
                        default:
                            assert(false);
                        }
                    }
                    else if (leftResult.type == CalcResult::ARRAY) {
                        assert(rightResult.type == CalcResult::CONSTANT);
                        // One Array operand and one constant operand.
                        res.type = CalcResult::ARRAY;
                        res.array.resizeErase(leftResult.array.height(), leftResult.array.width());
                        const float c = rightResult.constant;
                        switch (node->type) {
                        case Parser::Node::ADD:
                            CALCRESULT(leftResult.array[y][x], +, c)
                            break;
                        case Parser::Node::SUB:
                            CALCRESULT(leftResult.array[y][x], -, c)
                            break;
                        case Parser::Node::MULT:
                            CALCRESULT(leftResult.array[y][x], *, c)
                            break;
                        case Parser::Node::DIV:
                            CALCRESULT(leftResult.array[y][x], /, c)
                            break;
                        default:
                            assert(false);
                        }
                    }
                    else if (rightResult.type == CalcResult::ARRAY) {
                        assert(leftResult.type == CalcResult::CONSTANT);
                        // One Array operand and one constant operand.
                        res.type = CalcResult::ARRAY;
                        res.array.resizeErase(rightResult.array.height(), rightResult.array.width());
                        const float c = leftResult.constant;
                        switch (node->type) {
                        case Parser::Node::ADD:
                            CALCRESULT(c, +, rightResult.array[y][x])
                            break;
                        case Parser::Node::SUB:
                            CALCRESULT(c, -, rightResult.array[y][x])
                            break;
                        case Parser::Node::MULT:
                            CALCRESULT(c, *, rightResult.array[y][x])
                            break;
                        case Parser::Node::DIV:
                            CALCRESULT(c, /, rightResult.array[y][x])
                            break;
                        default:
                            assert(false);
                        }
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
                size_t hashPos = targetFileName.find("#");
                if(hashPos != string::npos)
                    targetFileName.replace(hashPos, 1, patch);
                cout << "computing " << targetFileName << "               \r";
                CalcResult res;
                std::function<void(const Parser::Node*)> closed = [&](const Parser::Node* node) { evaluationFunc(node, patch, res); };
                root->right->evaluate(closed);
                assert(res.type == CalcResult::ARRAY);
                writeRGB(targetFileName.c_str(), res.array[0], res.array.width() / 3, res.array.height(), compression);
            });
    } else {
        cout << "error parsing expression: " << p.getErrorMessage() << "\n";
    }
}