#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "lib/stb_image.h"
#include "lib/stb_image_resize2.h"

using namespace std;

//Constants
const string fontsizesFile = "charsizes.txt";

//Checks if a path is relative or absolute. If relative, appends it to current working directory path
string get_full_image_path(const string& filename) 
{// Precondition: !filename.empty();
    if (filename[0] == '/') {
        return filename; // Absolute path, return as is
    }

    // Get the current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return string(cwd) + "/" + filename;
    } else {
        cerr << "Error: Unable to get current working directory!" << endl;
        return filename; // Fallback to default behavior
    }
}

//Default values that the user can change
const string imagefileDefault = "img/madeline.png";
const int resXDefault = 128;
const string outputDefault = "output/output.txt";
const bool verboseDefault = false;
const int no_of_ascii_default = 4;
const bool invertDefault = false;
const bool terminalDefault = false;

string ascii_chars; //Palette of characters for art, sorted in order of decreasing brightness (gets reversed when invert is true)

//Contains all configurations the user can alter using arguments
struct config{
    string filename;
    int resX;
    string output_file;
    bool verbose;
    int no_of_ascii;
    bool invert;
    bool terminal;
    int resY;

    // Constructor to initialize the default values
    config(): 
        filename(imagefileDefault),
        resX(resXDefault),
        output_file(outputDefault),
        verbose(verboseDefault),
        no_of_ascii(no_of_ascii_default),
        invert(invertDefault),
        terminal(terminalDefault) {}
};

//Return status of parse_args. 
enum status{
    def, //All went well
    err, //Invalid command or arguments
    h,   //'help' entered, program must exit
};

//Self-explanatory
void print_help() {
    cout << "Usage: ascii_art [options]\n"
         << "Options:\n"
         << "  -h, --help               Show this help message and exit\n"
         << "  -f, --file FILE          Input image file (default: "<< imagefileDefault <<")\n"
         << "  -r, --res RES            Horizontal resolution of ASCII art (default: "<< resXDefault <<")\n"
         << "  -o, --output FILE        Output ASCII art file (default: "<< outputDefault << ")\n"
         << "  -v, --verbose            Do verbose logging (default: " << ((verboseDefault)?("true"):("false")) << ")\n"
         << "  -#, --no_of_chars        Amount of ascii characters to use (default: "<< no_of_ascii_default <<")\n"
         << "  -i, --invert             Inverts brightness values(default:"<< ((invertDefault)?("true"):("false")) << ")\n"
         << "  -c, --chars              Ascii characters to use. Overrides default ascii character selection (default: none)\n"
         << "  -t, --terminal           Output to terminal aswell as output file(default:"<< ((invertDefault)?("true"):("false")) <<")\n";
    return;
}

//Parses commandline arguments into config struct settings, returns a status that determines if program should exit
status parse_args(config &settings, int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return h;

        } else if (arg == "--file" || arg == "-f") {
            if (i + 1 < argc) settings.filename = get_full_image_path(argv[++i]);
            else { cerr << "Error: No file specified after " << arg << endl; return err; }

        } else if (arg == "--res" || arg == "-r") {
            if (i + 1 < argc) settings.resX = stoi(argv[++i]);
            else { cerr << "Error: No resolution specified after " << arg << endl; return err; }

        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) settings.output_file = "output/" + string(argv[++i]);
            else { cerr << "Error: No output file specified after " << arg << endl; return err; }

        } else if (arg == "--verbose" || arg == "-v") {
            settings.verbose = true;

        } else if (arg == "--invert" || arg == "-i") {
            settings.invert = true;

        }else if (arg == "--terminal" || arg == "-t") {
            settings.terminal= true;

        } else if (arg == "--no_of_chars" || arg =="-#") {
            if (i + 1 < argc) settings.no_of_ascii = stoi(argv[++i]);
            else { cerr << "Error: No resolution specified after " << arg << endl; return err; }

        } else if (arg == "--chars" || arg == "-c") {
            if (i + 1 < argc) ascii_chars = argv[++i];
            else { cerr << "Error: No characters specified after " << arg << endl; return err; }

        } else {
            cerr << "Error: Unknown argument " << arg << endl; return err;
        }
    }
    return def;
}

//Reads vector of character coverage from fontsizesFile
vector<pair<char, int> > read_char_coverage() {
    vector<pair<char, int> > res;

    ifstream infile(fontsizesFile);
    if (!infile.is_open()) { cerr << fontsizesFile << " could not be opened." << endl; return res; }
    
    string line;
    while (getline(infile, line)) {

        char first = line[0];;
        int second= stoi(line.substr(2)); // Convert from index 2 onwards

        //Create temporary pair. res.push_back({first, second}); doesn't work
        pair<char, int> tmp;
        tmp.first = first;
        tmp.second = second;

        res.push_back(tmp);
    }

    return res;
}

//Finds and returns character with paired int closest to ideal_val using binary search.
char find_nearest(vector<pair<char, int> > data, int ideal) {
    //Function setup
    char best;
    int min = 0;
    int max = data.size() - 1;
    int middle = (min + max) / 2;

    //Initial guess
    int error = abs(data.at(middle).second - ideal);

    //Binary search
    while (min <= max) {

        int new_error = abs(data.at(middle).second - ideal);
        
        if (new_error < error) {
            error = new_error;
            best = data.at(middle).first;
        }
        
        if (ideal < data.at(middle).second) {
            min = middle + 1;  // ideal is less, move window right
        } else {
            max = middle - 1;  // ideal is greater, move window left
        }
        middle = (min + max) / 2;
    }
    
    return best;
}

//Figures out string of characters to use as palette of length 'chars' 
string figure_out_chars(int chars) {
    string res = "";
    
    vector<pair<char, int> > char_coverages = read_char_coverage();
    const int max = char_coverages.at(0).second; //Its sorted to the first element is the highest one
    const int ideal_val_const = max / (chars - 1); //We loop 0 through chars-1, so chars-1 must correspond to max
    
    for (int i = 0; i < chars; i++) res += find_nearest(char_coverages, ideal_val_const * i);
    

    return res;
}

status retrieve_and_process_image(config& settings, unsigned char** data_out) {
    int width, height, channels;
    const int comps = 1; // Grayscale

    string full_image_path = get_full_image_path(settings.filename);

    // Load image
    unsigned char* data_tmp = stbi_load(full_image_path.c_str(), &width, &height, &channels, comps);
    if (!data_tmp) {
        cerr << "Failed to load image: " << full_image_path << endl;
        return err;
    }
    if (settings.verbose) cout << "Image successfully loaded" << endl;

    // Compute new vertical while maintaining aspect ratio
    settings.resY = static_cast<int>(settings.resX * (static_cast<float>(height) / width) * 0.45);

    *data_out = (unsigned char*)malloc(settings.resX * settings.resY * comps);
    if (!*data_out) {
        cerr << "Failed to allocate memory for resized image." << endl;
        stbi_image_free(data_tmp);
        return err;
    }

    // Resize the image
    stbir_resize(data_tmp, width, height, 0, *data_out, settings.resX, settings.resY, 0,
                 STBIR_1CHANNEL, STBIR_TYPE_UINT8,
                 STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT);

    if (settings.verbose) cout << "Image successfully resized" << endl;

    // Free original image data
    stbi_image_free(data_tmp);

    return def;
}

status produce_ascii(config settings, unsigned char* data){
    ofstream outFile(settings.output_file);
    if (!outFile.is_open()) {
        cerr << "Failed to open output file: " << settings.output_file << endl;
        free(data);
        return err;
    }

    for (int i = 0; i < settings.resY; i++) {
        for (int j = 0; j < settings.resX; j++) {

            unsigned char pixel_value = data[i * settings.resX + j];
            char ascii_char = ascii_chars[pixel_value * ascii_chars.size() / 256];

            if(settings.terminal) cout << ascii_char;

            outFile << ascii_char;
        }
        if(settings.terminal) cout << endl;
        outFile << endl;
    }

    // Cleanup
    free(data);
    outFile.close();
    if (settings.verbose) cout << "ASCII art saved to '" << settings.output_file << "'!" << endl;

    return def;
}   

int main(int argc, char* argv[]) {
    // Load default parameters
    config settings;

    // Parse command-line arguments (optional)
    status stat = parse_args(settings, argc, argv);
    switch(stat){
        case err: return 1;
        case h: return 0;
        case def: break;
    }

    if (ascii_chars.empty()) ascii_chars = figure_out_chars(settings.no_of_ascii);

    if (settings.verbose) cout << "selected ascii character palette: " << ascii_chars << endl;
    
    if (settings.invert) reverse(ascii_chars.begin(), ascii_chars.end());
    
    unsigned char* data = nullptr;
    stat = retrieve_and_process_image(settings, &data);
    switch(stat){
        case err: return 1;
        case h: return 0;
        case def: break;
    }
    
    stat = produce_ascii(settings, data);
    switch(stat){
        case err: return 1;
        case h: return 0;
        case def: break;
    }
    
    return 0;
}