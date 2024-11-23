#include <iostream>
#include <chrono>
#include <thread>

#define buffer_size 6000

void delay(int milliseconds){
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

int main(){
    while (true){
        std::string buffer = " ";
        std::string gps_data = "20:56:34+00:00,51.08472666666667,-114.128815,167.5";

        int i = 0;
        while (i < 10)// write after 10 buffer iterations
        {
            buffer = buffer + gps_data;
            delay(1000);
            std::cout << "Buffer: " << buffer << "\n\n\n" << "Buffer Length: " << buffer.length() << "\n\n";
            i++;
        }

        // pretend to write the buffer to the SD card

        // clear the buffer
        buffer = "";

        std::cout << "cleared bufffer" << std::endl;
    }   
    return 0;
}