//IMPORTS
#include "httplib.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "json.hpp"

int main() {
    //PORT FOR CLIENT TO CONNECT TO
    httplib::Client cli("localhost", 2180);

    //CREATE JSON OBJECT
    nlohmann::json j = {
            {"source", "block40"},
            {"data", "YOOO"},
            {"target", "21e8"}};

    //CHECK IF CLIENT WANTS TO SEND REQUEST OR RECIEVE DATA 
    std::string request; 
    std::cout << "Enter 'S' to send mine data, Enter 'R' to recieve mined data: ";
    std::cin >> request;

    //SEND JSON DATA
    if (request == "S") {
        auto res = cli.Post("/api/v2/mine", j.dump(), "hashes/json");
    }

    //RECIEVE MINED DATA
    if (request == "R") {
        auto res = cli.Get("/api/v2/filter/21e8");
        std::string body;
        std::cout << res->body << std::endl;
    }
};