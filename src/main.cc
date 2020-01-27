#include <iostream>
#include <string>
#include <vector>

#include "parser.h"

using namespace std;

void displayHelp() {
    cout << "Usage text.\n";
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
            return 0;
        }
    }

    Parser p(expression);
    if(p.isValid()) {
        cout << p.getRoot()->toString() << "\n";
		vector<string> filePaths;
		std::function<void(const Parser::Node* node)> evaluationFunc;
		evaluationFunc = [&](const Parser::Node* node) {
			if (node->type == Parser::Node::FILEPATH) {
				filePaths.push_back(node->path);
			}
			if (node->left)
				node->left->evaluate(evaluationFunc);
			if (node->right)
				node->right->evaluate(evaluationFunc);
		};
		p.getRoot()->evaluate(evaluationFunc);
		for (int i = 0; i < filePaths.size(); i++) {
			cout << "\n" << filePaths[i] << "\n";
		}
    } else {
        cout << "error: " << p.getErrorMessage() << "\n";
    }
}