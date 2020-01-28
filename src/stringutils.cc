#include "stringutils.h"

// Removes leading and trailing whitespace from string.
std::string trim(const std::string& s) {
	std::string tmp = s.substr(0, s.find_last_not_of(" ") + 1);
	tmp = tmp.substr(tmp.find_first_not_of(" "));
	return tmp;
}

// Splits string at delimiter into array.
std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
	std::string s = str;
	size_t delimiterPos = s.find(delimiter);
	if (delimiterPos == std::string::npos) {
		s = trim(s);
		return std::vector<std::string>(1, s);
	}
	std::string left = s.substr(0, delimiterPos);
	left = trim(left);
	std::string right = s.substr(delimiterPos + 1, std::string::npos);
	right = trim(right);
	std::vector<std::string> res(1, left);
	std::vector<std::string> rightElements = split(right, delimiter);
	res.insert(res.end(), rightElements.begin(), rightElements.end());
	return res;
}