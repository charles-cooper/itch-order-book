clang++ -ferror-limit=2 \
	-g -O3 -march=native -Wall -Wextra -Wswitch-enum -std=c++1y \
	main.cpp bufferedreader.cpp
