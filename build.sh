clang++ -ferror-limit=2 \
	-g -O0 -march=native -Wall -Wextra -Wswitch-enum -std=c++1y \
	main.cpp bufferedreader.cpp
