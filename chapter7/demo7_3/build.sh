# compile the C++ source file
g++ -c demo7_3.cpp -o main.o

# link the object files
g++ main.o -o app.exe -mwindows -luser32 -lgdi32 -lwinmm \
                      -luuid -lole32 \
                      -lddraw -ldxguid

# clean up intermediate files
rm main.o

# run the application
./app.exe