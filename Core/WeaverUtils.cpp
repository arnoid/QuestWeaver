//
// Created by michael on 11.10.15.
//

#include <Core/WeaverUtils.h>
#include <fstream>
#include <sstream>

using namespace std;
using namespace weave;
using namespace Json;

void weave::replaceAll(string *str, const string &from, const string &to) {
    if (from.empty()) {
        return;
    }
    size_t start_pos = 0;
    while ((start_pos = str->find(from, start_pos)) != string::npos) {
        str->replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

bool weave::replace(string *str, const string &from, const string &to) {
    size_t start_pos = str->find(from);
    if (start_pos == string::npos) {
        return false;
    }
    str->replace(start_pos, from.length(), to);
    return true;
}

string weave::htmlEncloseWithTag(const string &str, const string &tag) {
    return htmlEncloseWithTag(str, tag, vector<string>());
}

string weave::htmlEncloseWithTag(const string &str, const string &tag, const vector<string> &classes) {
    if (tag == "") {
        return str;
    }
    stringstream html;
    html << "<" << tag;
    if (classes.size() > 0) {
        html << " class=\"";
        for (uint64_t i = 0; i < classes.size(); i++) {
            html << classes[i];
            if (i < classes.size() - 1) {
                html << " ";
            }
        }
        html << "\"";
    }
    html << ">" << str;
    html << "</" << tag << ">";
    return html.str();
}

Value weave::readJsonFromFile(const char *fileName, const Directories &dirs) {
    Value root;
    CharReaderBuilder readBuilder;
    readBuilder["collectComments"] = false;
    readBuilder["rejectDupKeys"] = true;
    string errorMessage;
    ifstream inStream;

    string modDir(dirs.modDirectory);
    string dataDir(dirs.templateDirectory);

    // try to use the mods directory
    const char *moddedFile = modDir.append(fileName).c_str();
    inStream.open(moddedFile, ios::in);
    if (inStream.fail()) {
        // try to use the regular directory
        const char *templateFile = dataDir.append(fileName).c_str();
        inStream.open(templateFile, ios::in);
        if (inStream.fail()) {
            // try to open the file as it is
            string currentDir("./");
            const char *localFile = currentDir.append(fileName).c_str();
            inStream.open(localFile, ios::in);
            if (inStream.fail()) {
                string errorMsg =
                        string("Unable to find file in any of the following directories: [., ") +
                        dirs.templateDirectory +
                        ", " + dirs.modDirectory + "]";
                throw ContractFailedException(errorMsg);
            }
        }
    }

    if (!parseFromStream(readBuilder, inStream, &root, &errorMessage)) {
        cerr << "Error parsing template file: " << errorMessage << endl;
        throw ContractFailedException(errorMessage);
    }

    return root;
}
