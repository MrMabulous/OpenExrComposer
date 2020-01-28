#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <filesystem>

#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfNamespace.h>

#include "parser.h"
#include "stringutils.h"

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

struct CalcResult {
	enum CalcResultType {INVALID, ARRAY, CONSTANT};
	CalcResult() : type(INVALID) {}
	CalcResultType type;
	Array2D<float> array;
	float constant;
};

void displayHelp() {
    cout << "Usage text.\n";
}

void
writeRGB(const char fileName[],
	const float *rgbPixels,
	int width,
	int height)
{

	Header header(width, height);
	header.channels().insert("R", Channel(IMF::FLOAT));
	header.channels().insert("G", Channel(IMF::FLOAT));
	header.channels().insert("B", Channel(IMF::FLOAT));

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

    // Loop over remaining command-line args
    for (vector<string>::iterator i = args.begin()+1; i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            displayHelp();
            return 0;
        } else { 
            cout << "unknown argument " <<  *i << "\n";
            displayHelp();
            return 1;
        }
    }

	//string expression = "C:\\Users\\Matthias\\Dropbox\\projects\\OpenExrComposer\\OpenExrComposer\\x64\\Release\\ReflSpec.exr = C:\\Users\\Matthias\\Dropbox\\projects\\OpenExrComposer\\OpenExrComposer\\x64\\Release\\first_floor0000.VRayReflection.exr + C:\\Users\\Matthias\\Dropbox\\projects\\OpenExrComposer\\OpenExrComposer\\x64\\Release\\first_floor0000.VraySpecular.exr";

    Parser p(expression);
    if(p.isValid()) {
        cout << p.getRoot()->toString() << "\n";
		vector<string> inputFilePaths;
		string outputFilePath;
		std::function<void(const Parser::Node* node)> printFunc;
		printFunc = [&](const Parser::Node* node) {
			if (node->type == Parser::Node::INPUTFILEPATH) {
				inputFilePaths.push_back(node->path);
			}
			else if (node->type == Parser::Node::OUTPUTFILEPATH) {
				outputFilePath = node->path;
			}
			if (node->left)
				node->left->evaluate(printFunc);
			if (node->right)
				node->right->evaluate(printFunc);
		};
		p.getRoot()->evaluate(printFunc);
		for (int i = 0; i < inputFilePaths.size(); i++) {
			cout << inputFilePaths[i] << "\n";
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
				vector<string> nameSplit = split(fileNameString, "#");
				for (const auto & entry : filesystem::directory_iterator(folderPath)) {
					std::cout << entry.path() << std::endl;
				}
			}
			else {
				if (!filesystem::exists(filePath)) {
					cout << "error: " << filePath.string() << " does not exist.\n";
					return 1;
				}
			}
		}

		std::map<const Parser::Node*, CalcResult> results;
		std::function<void(const Parser::Node* node)> evaluationFunc = [&](const Parser::Node* node) {
			CalcResult& res = results[node];
			switch (node->type) {
			case Parser::Node::INPUTFILEPATH:
				int width, height;
				res.type = CalcResult::ARRAY;
				readRGB(node->path.c_str(), res.array, width, height);
				return;
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
				node->left->evaluate(evaluationFunc);
				node->right->evaluate(evaluationFunc);
				CalcResult& leftResult = results[node->left];
				CalcResult& rightResult = results[node->right];
				assert(leftResult.type != CalcResult::INVALID);
				assert(rightResult.type != CalcResult::INVALID);
				if (leftResult.type == CalcResult::CONSTANT && rightResult.type == CalcResult::CONSTANT) {
					res.type = CalcResult::CONSTANT;
					res.constant = results[node->left].constant * results[node->right].constant;
				}
				else if (leftResult.type == CalcResult::ARRAY && rightResult.type == CalcResult::ARRAY) {
					res.type = CalcResult::ARRAY;
					res.array.resizeErase(leftResult.array.height(), leftResult.array.width());
					assert(leftResult.array.width() == rightResult.array.width() && leftResult.array.height() == rightResult.array.height());
					switch (node->type) {
					case Parser::Node::ADD:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] + rightResult.array[y][x];
							}
						}
						break;
					case Parser::Node::SUB:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] - rightResult.array[y][x];
							}
						}
						break;
					case Parser::Node::MULT:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] * rightResult.array[y][x];
							}
						}
						break;
					case Parser::Node::DIV:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] / rightResult.array[y][x];
							}
						}
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
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] + c;
							}
						}
						break;
					case Parser::Node::SUB:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] - c;
							}
						}
						break;
					case Parser::Node::MULT:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] * c;
							}
						}
						break;
					case Parser::Node::DIV:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = leftResult.array[y][x] / c;
							}
						}
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
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = c + rightResult.array[y][x];
							}
						}
						break;
					case Parser::Node::SUB:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = c - rightResult.array[y][x];
							}
						}
						break;
					case Parser::Node::MULT:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = c * rightResult.array[y][x];
							}
						}
						break;
					case Parser::Node::DIV:
						for (int y = 0; y < res.array.height(); y++) {
							for (int x = 0; x < res.array.width(); x++) {
								res.array[y][x] = c / rightResult.array[y][x];
							}
						}
						break;
					default:
						assert(false);
					}
				}
				// free memory from left and right node calculations.
				results.erase(node->left);
				results.erase(node->right);
				return;
			}
			default:
				assert(false);
			}
		};
		const Parser::Node* root = p.getRoot();
		cout << "computing...\n";
		root->right->evaluate(evaluationFunc);
		assert(results.size() == 1);
		assert(results.count(root->right) == 1);
		CalcResult& res = results[root->right];
		assert(res.type == CalcResult::ARRAY);
		cout << "writing result to: " << root->left->path << "\n";
		writeRGB(root->left->path.c_str(), res.array[0], res.array.width() / 3, res.array.height());
    } else {
        cout << "error: " << p.getErrorMessage() << "\n";
    }
}