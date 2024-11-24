#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm> 
#include <ft2build.h>
#include FT_FREETYPE_H

const int pixelheight = 1024;
const int divisor = static_cast<int>(pixelheight * 0.442 * pixelheight);

const char* defaultFontPath = "/System/Library/Fonts/SFNSMono.ttf";
const char* defaultOutputFile = "charsizes.txt";

bool comparePairs(const std::pair<char, int>& a, const std::pair<char, int>& b) {
    return a.second > b.second; // Sort in descending order of values
}

int coverage(const char* fontPath, char s) {
    // Initialize FreeType library
    FT_Library library;
    if (FT_Init_FreeType(&library)) {
        std::cerr << "Error: Could not initialize FreeType library\n";
        return -1;
    }

    // Load a font
    FT_Face face;
    if (FT_New_Face(library, fontPath, 0, &face)) {
        std::cerr << "Error: Could not load font from " << fontPath << "\n";
        FT_Done_FreeType(library);
        return -1;
    }

    // Set the font size
    FT_Set_Pixel_Sizes(face, 0, pixelheight);

    // Load the glyph for the character
    if (FT_Load_Char(face, s, FT_LOAD_RENDER)) {
        std::cerr << "Error: Could not load character '" << s << "'\n";
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return -1;
    }

    // Access the glyph's bitmap
    FT_Bitmap& bitmap = face->glyph->bitmap;
    int width = bitmap.width;
    int height = bitmap.rows;

    if (width == 0 || height == 0) {
        // Character has no visible pixels (e.g., space)
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 0;
    }

    // Calculate the coverage
    int filledPixels = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Safeguard against pitch being larger than width
            if (bitmap.buffer[y * bitmap.pitch + x] > 0) {
                ++filledPixels;
            }
        }
    }

    int percentage = (filledPixels * 10000) / divisor;

    // Clean up
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return percentage;
}

void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "Generates character coverage percentages for the specified font file.\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help       Show this help message and exit\n";
    std::cout << "  -f, --font       Specify the path to the font file (default: " << defaultFontPath << ")\n";
}


int main(int argc, char* argv[]) {
	 const char* fontPath = defaultFontPath;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--font" || arg == "-f") {
            if (i + 1 < argc) fontPath = argv[++i];
            else {
                std::cerr << "Error: No font file specified after " << arg << "\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown argument " << arg << "\n";
            return 1;
        }
    }


    // Store characters and their coverage values
    std::map<char, int> charValues;
    for (int i = 32; i < 127; i++) {
        char c = static_cast<char>(i);
        int cov = coverage(fontPath, c);
        if (cov >= 0) { // Exclude characters with errors
            charValues[c] = cov;
        }
    }

    // Sort characters by coverage
    std::vector<std::pair<char, int> > sortedChars(charValues.begin(), charValues.end());
    std::sort(sortedChars.begin(), sortedChars.end(), comparePairs);

    // Write sorted characters with their coverage to a file
    std::ofstream outFile(defaultOutputFile);
    if (!outFile) {
        std::cerr << "Error: Could not open output file: " << defaultOutputFile << "\n";
        return 1;
    }

    for (int i=0;i<sortedChars.size();i++) {
		std::pair<char, int> pair = sortedChars.at(i);
        //std::cout << pair.first << ", " << pair.second << ",\n";
		outFile << pair.first << " " << pair.second << "\n";
    }

    outFile.close();
    std::cout << "Results written to " << defaultOutputFile << "\n";

    return 0;
}

