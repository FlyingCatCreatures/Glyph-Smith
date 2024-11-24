install:
	clang++ asciiart.cpp -o asciiart
	clang++ -o charcov charcov.cpp -lfreetype -I/opt/homebrew/include/freetype2 -L/opt/homebrew/lib
	sudo cp charcov /usr/local/bin/charcov
	sudo cp asciiart /usr/local/bin/asciiart

develop:
	clang++ asciiart.cpp -o asciiart


charcov:
	clang++ -o charcov charcov.cpp -lfreetype -I/opt/homebrew/include/freetype2 -L/opt/homebrew/lib
