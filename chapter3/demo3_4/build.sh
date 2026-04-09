# compile the resource file
windres DEMO3_4.rc -o app_res.o

# compile the C++ source file
g++ -c DEMO3_4.cpp -o main.o

# link the object files
g++ main.o app_res.o -o app.exe -mwindows -luser32 -lgdi32 -lwinmm

# clean up intermediate files
rm main.o app_res.o

# run the application
./app.exe