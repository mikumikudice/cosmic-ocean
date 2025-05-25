set -xe
cd src
gcc -Wall -Werror -o ../bin/main main.c tigr.c -lm -lGLU -lGL -lX11
cd ..

