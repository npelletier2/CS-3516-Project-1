#include <iostream>
#include <string>
#include <array>
#include <memory>

std::string decode_qr(std::string filename){
    //TODO
    std::array<char, 128> buffer;
    std::string result = "";
    std::string cmd = "java -cp ZXing/javase.jar:ZXing/core.jar com.google.zxing.client.j2se.CommandLineRunner " + filename;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose); 

    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
        result += buffer.data();
    }

    std::cout << result << std::endl;

    return result;
}

int main(){
    std::string result = decode_qr("client/qr_testing/project1_qr_code.png");
}
