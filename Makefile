install:
	#clang++ asciiart.cpp -o asciiart -I/opt/homebrew/Cellar/cairo/1.18.2/include/cairo -L/opt/homebrew/Cellar/cairo/1.18.2/lib -lcairo
	clang++ asciiart.cpp -o asciiart
	clang++ -o charcov charcov.cpp -lfreetype -I/opt/homebrew/include/freetype2 -L/opt/homebrew/lib
	sudo cp asciiart /usr/local/bin/asciiart

develop:
	clang++ asciiart.cpp -o asciiart