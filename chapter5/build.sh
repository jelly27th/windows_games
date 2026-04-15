# base COM program
g++ demo5_1.cpp -o com_example.exe -luser32 -lgdi32 -luuid -lole32

# run the application
./com_example.exe