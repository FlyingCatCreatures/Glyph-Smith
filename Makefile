install:
	#clang++ asciiart.cpp -o asciiart -I/opt/homebrew/Cellar/cairo/1.18.2/include/cairo -L/opt/homebrew/Cellar/cairo/1.18.2/lib -lcairo
	clang++ asciiart.cpp -o asciiart 
	clang++ -o charcov charcov.cpp -lfreetype -I/opt/homebrew/include/freetype2 -L/opt/homebrew/lib
	sudo cp asciiart /usr/local/bin/asciiart

develop:
	clang++ asciiart.cpp -o asciiart -Wall -Wextra -Wpedantic -Wshadow -Wuninitialized -Wconversion -Werror -fsanitize=address --analyze | grep -v stb

profile:
	clang++ -g asciiart.cpp -o asciiart -fprofile-instr-generate -fcoverage-mapping
	sudo cp asciiart /usr/local/bin/asciiart
	#after running program run:
	#llvm-profdata merge -sparse default.profraw -o default.profdata
	#llvm-cov show --ignore-filename-regex='.*stb.*' ./asciiart -instr-profile=default.profdata