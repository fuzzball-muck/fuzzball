/*
* Copyright (c) 2014 by Fuzzball Software
* under the GNU Public License
*/

#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
using namespace std;

// Define if you want the timezone to show UTC. Windows doesn't do
// abbreviations for timezone, so this is the only way to get it shortened.
#undef USE_UTC

// Filenames
#define TPL_FILE "version.tpl"
#define VERSION_FILE "version.cpp"

// Prototypes
unsigned int getgeneration();
string getnow();


int main(int argc, char* argv[])
{
	unsigned int generation = 0;
	string creation = "";

	generation = getgeneration();
	creation = getnow();

	ifstream tpl;
	tpl.open(TPL_FILE, ifstream::in);

	if (!tpl) {
		cerr << "error: could not open " << TPL_FILE << "." << endl;
		return -1;
	}

	ofstream ver;
	ver.open(VERSION_FILE, ofstream::trunc | ofstream::out);

	if (!ver) {
		cerr << "error: could not open " << VERSION_FILE << "." << endl;
		return -1;
	}

	const string genline = "$generation";
	const string creline = "$creation";

	while (tpl.good() && !tpl.eof() && ver.good()) {
		string line = "";
		getline(tpl, line);

		size_t pos = line.find(genline);
		if (pos != string::npos) {
			string left = line.substr(0, pos);
			string right = line.substr(pos + genline.length());
			line = left + to_string(generation) + right;
		}

		pos = line.find(creline);
		if (pos != string::npos) {
			string left = line.substr(0, pos);
			string right = line.substr(pos + creline.length());
			line = left + creation + right;
		}

		ver << line << endl;
	}

	// TODO: Generate the do_showextver function instead of using TPL

	ver.close();
	tpl.close();

	return 0;
}

// Return a time in format of "Mon Apr 14 2008 at 12:01:17 EDT"
string getnow() {
	char buf[1024];
	time_t t = time(NULL);
	struct tm info;

#ifdef USE_UTC
	gmtime_s(&info, &t);
	strftime(buf, sizeof(buf), "%a %b %d %Y at %H:%M:%S UTC", &info);
#else
	localtime_s(&info, &t);
	strftime(buf, sizeof(buf), "%a %b %d %Y at %H:%M:%S %Z", &info);
#endif

	return string(buf);
}

// Get the generation value from the existing cpp file.
unsigned int getgeneration() {
	unsigned int generation = 0;
	const string GENDEF = "#define generation";

	ifstream v;
	v.open(VERSION_FILE, ifstream::in);

	if (!v)
		return 0;

	while (v.good() && !v.eof()) {
		string line = "";
		getline(v, line);

		size_t pos = line.find(GENDEF);
		if (pos == string::npos)
			continue;

		string genstring = line.substr(pos + GENDEF.length());
		size_t start = 0, end = 0;
		for (start = 0; start < genstring.length(); start++)
			if (isdigit(genstring[start])) break;
		for (end = start; end < genstring.length(); end++)
			if (!isdigit(genstring[end])) break;

		if (start < genstring.length() && start < end) {
			genstring = genstring.substr(start, end - start);
			return stoi(genstring) + 1;
		}		
	}

	return generation;
}

