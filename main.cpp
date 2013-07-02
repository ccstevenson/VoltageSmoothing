#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

int vt = -1;
int width = -1;
int pulse_delta = -1;
float drop_ratio = -1;
int below_drop_ratio = -1;

void parseIni(string line) {
    string key;
    string valueString;
    float value;

    if (line[0] != '#') { // The line is not a comment. Process it.
        stringstream linestream(line);

        getline(linestream, key, '=');
        getline(linestream, valueString);

        stringstream(valueString) >> value; // May want to validate proper input.

        if (key == "vt")
            vt = value;
        else if (key == "width")
            width = value;
        else if (key == "pulse_delta")
            pulse_delta = value;
        else if (key == "drop_ratio")
            drop_ratio = value;
        else if (key == "below_drop_ratio")
            below_drop_ratio = value;
        else {
            cout << ".ini file specified a parameter that is not supported.";
            exit(-1);
        }
    }
}

void validateAllParametersSupplied() {
    if (vt == -1 || width == -1 || pulse_delta == -1 || drop_ratio == -1 || below_drop_ratio == -1) {
        cout << "the .ini file did not specify all required parameters.";
        exit(-1);
    }
}

vector<int> values;
vector<int> pulses;

void findArea(vector<int>& v) {
    for (int i = 0; i < v.size(); ++i) {
        if ((i < v.size()-1) && (v[i+1] - v[i] < width)) {
            cout << accumulate(values.begin() + v[i], values.begin() + v[i+1], 0) << " ";
        }
        else {
            cout << accumulate(values.begin() + v[i], values.begin() + v[i] + width, 0) << " ";
        }
    }
}

int err = 123;

void removeDefaultedValues(vector<int>& v) { // Values entered into the pulse array during handling.
    v.erase(remove(v.begin(), v.end(), 0), v.end());
    v.erase(remove(v.begin(), v.end(), 100), v.end());
    v.erase(remove(v.begin(), v.end(), err), v.end());
}

int fallBelowThreshhold;

bool cmp(int& num) {
    int* val = &num;
    int point = val[0];
    return (point < fallBelowThreshhold);
}

int pulsePosition;
int previousPulsePeakPosition;
int iPlus2; // The index at which we begin to search for decreasing values.
bool intraPulse = false;
vector<int> smoothValues;

int findPulse(int& num) {
    int* val = &num;
    pulsePosition++;
    int diff = val[2] - val[0];

    removeDefaultedValues(pulses);

    if (intraPulse == true && pulsePosition-1 >= iPlus2) { // Begin searching for a decreasing sample.
        if (val[0] > val[1]) { // The samples have begun to decrease. {
            previousPulsePeakPosition = pulsePosition-1;
            intraPulse = false;
        }
    }

    if ((diff > vt) && (intraPulse == false) && (diff != err)) { // If true, a pulse is found.
        int actualDelta = pulsePosition-1 - pulses.back();
        if (!pulses.empty() and actualDelta <= pulse_delta) {
            fallBelowThreshhold = smoothValues[previousPulsePeakPosition] * drop_ratio;
            int pointsExceedingFallBelowThreshold = count_if(smoothValues.begin() +
                                                         previousPulsePeakPosition, smoothValues.begin() +
                                                         pulsePosition, cmp) - 1;
            if (pointsExceedingFallBelowThreshold > below_drop_ratio) {
                pulses.pop_back();
            }
        }

        intraPulse = true;
        iPlus2 = pulsePosition+1;
        return pulsePosition-1;
    }
}

int smoothingFunction(int& num) {
    int* val = &num;
    // cout << "Original: " << *val << endl; // For debugging.
    int smoothVal = (val[-3] + 2*val[-2] + 3*val[-1] + 3*val[0] + 3*val[1] + 2*val[2] + val[3]) / 15;
    // cout << "Smoothed: " << smoothVal << endl; // For debugging.

    return smoothVal;

}

vector<int> smooth(vector<int> values) {
    vector<int> smoothValues(values);

    transform(smoothValues.begin()+4, smoothValues.end()-4, smoothValues.begin()+4, smoothingFunction);

    return smoothValues;
}

int main()
{
    system("dir /b *.dat > files.txt  2>nul");

    fstream ini("inifile.ini");

    if (!ini.good()) {
        cout << ".ini file not found.";
        return -1;
    }

    string line;
    while (getline(ini, line)) {
        parseIni(line);
    }

    validateAllParametersSupplied();

    // Now read the filenames from files.txt . . .
    fstream files("files.txt");
    string filename;
    while (getline(files, filename)) {
        fstream data(filename); // Open the .dat file.

        string valueString;
        int value;

        values.clear();
        while(getline(data, valueString)){ // Read all values from the .dat file into values.
            stringstream(valueString) >> value;
            values.push_back(-value);
        }

        smoothValues = smooth(values);
        pulses.clear();

        transform(smoothValues.begin(), smoothValues.end()-2, back_inserter(pulses), findPulse);
        pulsePosition = 0; // Reset.

        cout << filename << ": ";

        removeDefaultedValues(pulses);

        for_each(pulses.begin(), pulses.end(), [](int v){ cout << v << " "; });
        cout << endl;
        findArea(pulses);
        cout << endl;
    }

    return 0;
}
