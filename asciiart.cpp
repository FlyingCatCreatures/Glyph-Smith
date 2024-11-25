#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <atomic>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "lib/stb_image.h"
#include "lib/stb_image_resize2.h"

using namespace std;
using namespace chrono;

//Constants
const string fontsizesFile = "charsizes.txt";
const float framerate = 10.0;

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
const bool doOutputDefault = true;
const float rotateSpeedDefault = 0.0f;
const int rotations = 10;

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
    bool output;
    int resY;
    int channels;
    float rotateSpeed;

    // Constructor to initialize the default values
    config(): 
        filename(imagefileDefault),
        resX(resXDefault),
        output_file(outputDefault),
        verbose(verboseDefault),
        no_of_ascii(no_of_ascii_default),
        invert(invertDefault),
        terminal(terminalDefault),
        output(doOutputDefault),
        rotateSpeed(rotateSpeedDefault) {}
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
         << "  -h,              --help                  Show this help message and exit\n"
         << "  -f FILE,         --file FILE             Input image file (default: "<< imagefileDefault <<")\n"
         << "  -w RES,          --width RES             Horizontal resolution of ASCII art in characters(default: "<< resXDefault <<")\n"
         << "  -o FILE,         --output FILE           Output ASCII art file (default: "<< outputDefault << ")\n"
         << "  -v,              --verbose               Do verbose logging (default: " << ((verboseDefault)?("true"):("false")) << ")\n"
         << "  -#,              --no_of_chars           Amount of ascii characters to use (default: "<< no_of_ascii_default <<")\n"
         << "  -i,              --invert                Inverts brightness values(default:"<< ((invertDefault)?("true"):("false")) << ")\n"
         << "  -c,              --chars                 Ascii characters to use. Overrides default ascii character selection (default: none)\n"
         << "  -t,              --terminal              Output to terminal aswell as output file(default:"<< ((invertDefault)?("true"):("false")) <<")\n"
         << "  -r SPEED,        --rotate SPEED          Sets rotations per second to SPEED (default:"<< rotateSpeedDefault <<")\n"
         << "                                                  - Also enables terminal output and disables file output\n";
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

        } else if (arg == "--width" || arg == "-w") {
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

        } else if (arg == "--no_of_chars" || arg == "-#") {
            if (i + 1 < argc) settings.no_of_ascii = stoi(argv[++i]);
            else { cerr << "Error: No resolution specified after " << arg << endl; return err; }

        } else if (arg == "--chars" || arg == "-c") {
            if (i + 1 < argc) ascii_chars = argv[++i];
            else { cerr << "Error: No characters specified after " << arg << endl; return err; }

        } else if(arg == "--rotate" || arg == "-r") {
            if (i + 1 < argc) settings.rotateSpeed = stof(argv[++i]);
            else { cerr << "Error: No speed specified after " << arg << endl; return err; }
            settings.terminal= true;
            settings.output = false;
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

//Loads and processess image into 'data_out' according to 'settings'
status load_and_process_image(config& settings, unsigned char** data_out) {
    int width, height, channels;

    string full_image_path = get_full_image_path(settings.filename);

    // Load image
    unsigned char* data_tmp = stbi_load(full_image_path.c_str(), &width, &height, &channels, 0);

    if (!data_tmp) {
        cerr << "Failed to load image: " << full_image_path << endl;
        return err;
    }
    if (settings.verbose) cout << "Image successfully loaded" << endl;

    // Compute new vertical while maintaining aspect ratio
    settings.resY = static_cast<int>(settings.resX * (static_cast<float>(height) / width) * 0.442);
    settings.channels = channels;
    //*data_out = (unsigned char*)malloc(settings.resX * settings.resY * comps);
    *data_out = (unsigned char*)malloc(settings.resX * settings.resY * channels);


    // Determine the pixel layout based on the number of channels
    stbir_pixel_layout pixel_layout;
    switch (channels) {
        case 1: pixel_layout = STBIR_1CHANNEL; break;
        case 2: pixel_layout = STBIR_2CHANNEL; break;
        case 3: pixel_layout = STBIR_RGB; break;
        case 4: pixel_layout = STBIR_RGBA; break;
        default:
            cerr << "Unsupported number of channels: " << channels << endl;
            free(*data_out);
            stbi_image_free(data_tmp);
            return err;
    }

    // Resize the image
    stbir_resize(data_tmp, width, height, 0, *data_out, settings.resX, settings.resY, 0,
                 pixel_layout, STBIR_TYPE_UINT8,
                 STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT);

    if (settings.verbose) cout << "Image successfully resized" << endl;

    // Free original image data
    stbi_image_free(data_tmp);

    return def;
}

status produce_ascii(config settings, unsigned char* data) {
    ofstream outFile(settings.output_file);
    ostringstream buffer;
    ostringstream buffer2;

    for (int i = 0; i < settings.resY; i++) {
        string linebuff = "";
        string linebuff2 = "";
        for (int j = 0; j < settings.resX; j++) {
            // Calculate pixel index in the flattened data array
            int pixel_index = (i * settings.resX + j) * settings.channels;

            // Extract color components
            unsigned char r = 0, g = 0, b = 0;
            if (settings.channels == 1) {
                // Grayscale image: replicate the value for RGB
                r = g = b = data[pixel_index];
            } else if (settings.channels >= 3) {
                // RGB image
                r = data[pixel_index];
                g = data[pixel_index + 1];
                b = data[pixel_index + 2];
            } else {
                cerr << "Unsupported number of channels: " << settings.channels << endl;
                free(data);
                return err;
            }

            // Compute grayscale value for ASCII character mapping
            unsigned char grayscale_value = static_cast<unsigned char>(
                0.299f * r + 0.587f * g + 0.114f * b
            );

            // Map grayscale value to an ASCII character
            char ascii_char = ascii_chars[grayscale_value * ascii_chars.size() / 256];

            // Append the colored ASCII character to the line buffer
            if (settings.terminal)
                linebuff += "\033[38;2;" + to_string((int)r) + ";" +
                                    to_string((int)g) + ";" +
                                    to_string((int)b) + "m" + 
                                    ascii_char;

            // Write the plain ASCII character to the output file (no color)
            if (settings.output) linebuff2 += ascii_char;
        }
        if (settings.terminal) buffer << linebuff << "\033[0m" << endl;
        if (settings.output) buffer2 << linebuff2 << endl;
    }



    if(settings.terminal) cout << buffer.str();

    if(settings.output){ 
        ofstream outFile(settings.output_file);
        if (!outFile.is_open() && settings.output) {
            cerr << "Failed to open output file: " << settings.output_file << endl;
            free(data);
            return err;
        }

        outFile << buffer2.str();

        outFile.close();
    }
    if (settings.verbose) cout << "Colored ASCII art saved to '" << settings.output_file << "'!" << endl;

    return def;
}

unsigned char* rotate_image(const unsigned char* img, int width, int height, int channels, double theta) {
    int new_width = width;
    int new_height = height;
    unsigned char* rotated_img = (unsigned char*)malloc(new_width * new_height * channels);
    
    // Center of the image
    double cx = width / 2.0;
    double cy = height / 2.0;

    // Precompute sine and cosine
    double cos_theta = cos(theta);
    double sin_theta = sin(theta);

    // Initialize the new image to black (optional)
    fill(rotated_img, rotated_img + new_width * new_height * channels, 0);

    // Iterate over each pixel in the output image
    for (int y = 0; y < new_height; ++y) {
        for (int x = 0; x < new_width; ++x) {
            // Map (x, y) in the new image back to the original image
            double new_x = (x - cx) * cos_theta + (y - cy) * sin_theta + cx;
            double new_y = -(x - cx) * sin_theta + (y - cy) * cos_theta + cy;

            // Check if the coordinates are within bounds
            int src_x = static_cast<int>(new_x);
            int src_y = static_cast<int>(new_y);
            if (src_x >= 0 && src_x < width && src_y >= 0 && src_y < height) {
                for (int c = 0; c < channels; ++c) {
                    rotated_img[(y * new_width + x) * channels + c] =
                        img[(src_y * width + src_x) * channels + c];
                }
            }
        }
    }

    return rotated_img;
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
    stat = load_and_process_image(settings, &data);
    switch(stat){
        case err: return 1;
        case h: free(data); return 0;
        case def: break;
    }

    if (settings.rotateSpeed > 0) {
        double iterations_per_rotation = framerate / static_cast<double>(settings.rotateSpeed);
        double rotation_per_iteration = 2.0 * M_PI / iterations_per_rotation;
        int sum = 0;
        for (double theta = 0; theta < rotations * 2.0 * M_PI; theta += rotation_per_iteration) {
            steady_clock::time_point start = high_resolution_clock::now();
            stat = produce_ascii(settings, rotate_image(data, settings.resX, settings.resY, settings.channels, theta));
            switch(stat) {
                case err: free(data); return 1;
                case h: free(data); return 0;
                case def: break;
            }

            steady_clock::time_point end = high_resolution_clock::now();
            sum+= duration_cast<microseconds>(end-start).count();
            this_thread::sleep_for(milliseconds(static_cast<int>(1000.0 / framerate)) - (end - start));
        }

        produce_ascii(settings, data); //Final frame, so that the last frame is always precisely upright
        cout << "Average frametime: " << sum / (iterations_per_rotation*rotations) << " microseconds (" << sum / (1000*iterations_per_rotation * rotations)  << " ms)" << endl;

    } else {
        stat = produce_ascii(settings, rotate_image(data, settings.resX, settings.resY, settings.channels, 0));
        switch(stat) {
            case err: free(data); return 1;
            case h: free(data); return 0;
            case def: break;
        }
    }

    free(data);
    return 0;
}