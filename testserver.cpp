//IMPORTS
#include "httplib.h"
#include <picosha2.h>
#include <taskflow.hpp>
#include <sstream>
#include "json.hpp"
#include <sys/stat.h>
#include <cstring>
#include <unordered_map>
#include <cassert>
#include <iterator>
#include <vector>
#include <ctime>

//OMITTING PREFIXES FOR CONVENIENCE
using json = nlohmann::json;
using namespace httplib;

std::string mineData, target, sourceHashStr;
bool running = true;

void miner(uint64_t &nonceNum) {
    std::string prefix = "0000", nonce, resultHashStr, dataHashStr, hashStr, postHashStr, topHashStr;
    std::vector<std::string> hashVector;
    std::unordered_map<std::string, std::vector<std::string>> hashMap;
    hashVector.clear();
    json mined;
    try {
        std::ifstream i("hashes/hashMap.json");
        mined = json::parse(i);
    } catch (json::parse_error& ex){}
    if (mined[sourceHashStr].empty()) {
        mined[sourceHashStr] = hashVector;
    };
    std::vector<std::string> minedDataVec = mined[sourceHashStr];
    for (std::string s: minedDataVec) {
        hashVector.push_back(s);
    };

    std::vector<unsigned char> dataHash(picosha2::k_digest_size);
    picosha2::hash256(mineData.begin(), mineData.end(), dataHash.begin(), dataHash.end());
    dataHashStr = picosha2::bytes_to_hex_string(dataHash.begin(), dataHash.end());

    while ( prefix != target && running == true ) {

        std::stringstream ss; ss << nonceNum; ss >> nonce;

        hashStr = sourceHashStr + dataHashStr + target + "33ebd52a27fb61999f34cceb03eb4750058a5536eaa405df862d8f9c074d4e9d" + nonce;

        std::vector<unsigned char> hash(picosha2::k_digest_size);
        picosha2::hash256(hashStr.begin(), hashStr.end(), hash.begin(), hash.end());
        resultHashStr = picosha2::bytes_to_hex_string(hash.begin(), hash.end());

        prefix = resultHashStr.substr(0, target.length());

        nonceNum = nonceNum + 4;
    }
    
    if ( prefix == target ) {

        std::time_t result = std::time(nullptr);
        std::ostringstream oss; oss << "hashes/" << sourceHashStr << ".json";
        std::string hashFile = oss.str();
        std::ostringstream oss1; oss1 << "data/" << dataHashStr << ".txt";
        std::string dataFile = oss1.str();

        if (std::find(hashVector.begin(), hashVector.end(), resultHashStr) != hashVector.end()) {
        } else {
        hashVector.push_back(resultHashStr);
        hashMap[sourceHashStr] = hashVector; 

        //RETRIEVES THE KEY AND VALUES OF MAPPED ITEM
        /*std::unordered_map<std::string, std::vector<std::string>>::iterator i = hashMap.find(sourceHashStr);
        assert(i != hashMap.end());
        std::cout << "Key: " << i->first << '\n';
        for (std::string l: i->second)
        std::cout << " Value: " << l << '\n';*/

        json hM;
        json j;
        
        try {
            std::ifstream i("hashes/hashMap.json");
            i >> hM;
        } catch (json::parse_error& ex){}
        
        hM[sourceHashStr] = hashVector;
        std::ofstream open("hashes/hashMap.json");
        open << std::setw(4) << hM << std::endl;

        try {
            std::ifstream i(hashFile);
            i >> j;
        } catch (json::parse_error& ex){}

        j[resultHashStr] = {
            {"datahash", dataHashStr},
            {"n", nonce},
            {"rotation", resultHashStr},
            {"source", sourceHashStr},
            {"target", target},
            {"timestamp", result},
            {"user", "33ebd52a27fb61999f34cceb03eb4750058a5536eaa405df862d8f9c074d4e9d"}};

        std::ofstream o(hashFile);
        o << std::setw(4) << j << std::endl;

        std::ofstream myfile;
        myfile.open (dataFile);
        myfile << mineData;
        myfile.close();
        running = false;
        }
    }
    prefix = "0000";
}

int main() {
    tf::Executor executor;
    tf::Taskflow taskflow;

    //DEFINING THE SERVER
    httplib::Server svr;

    //SEND MINED DATA
    svr.Get(R"(/api/v2/index/(.*))", [&](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string source = req.matches[1];
        std::stack<std::string> stack;
        std::string top;
        std::string childFile;
        std::vector<std::string> minedDataVec;
        running = true;

        json mined;
        try {
            std::ifstream i("hashes/hashMap.json");
            mined = json::parse(i);
        } catch (json::parse_error& ex){}

        if ( !mined[source].empty() ) {
            std::vector<std::string> minedDataVec = mined[source];
            while ( minedDataVec.size() > 0 ) {
                for (std::string s: minedDataVec) {
                    stack.push(s);
                }
                for (std::string s: minedDataVec) {
                    if ( !mined[s].empty() ) {
                        minedDataVec = mined[s].get<std::vector<std::string>>();
                    } else {
                        minedDataVec.clear();
                    }
                }
            }
        }

        /*for (auto it = mined.begin(); it != mined.end(); ++it) {
            for (std::string s: it.value()) {
                responsePrefix = s.substr(0, prefix.length());
                if  ( responsePrefix == prefix ) {
                    responseList.push_back(s);
                }
            }
        }

        std::string response;
        for (std::string s: responseList) {
            response.append(s);
            response.append("\n");
        }*/

        std::ostringstream oss; oss << "hashes/" << source << ".json";
        std::string reqFile = oss.str();
        json result;
        try {
            std::ifstream i(reqFile);
            i >> result;
        } catch (json::parse_error& ex){}

        while ( !stack.empty() ) {
            top = stack.top();
            stack.pop();
            std::ostringstream oss; oss << "hashes/" << top << ".json";
            childFile = oss.str();

            json childResult;
            try {
                    std::ifstream i(childFile);
                    i >> childResult;
            } catch (json::parse_error& ex){}
            for (auto& [key, value] : childResult.items()) {
                result[key] = value;
            }
            std::ofstream o("hashes/GET-INDEX.json");
            o << std::setw(4) << result << std::endl;
        };

        std::string response = result.dump();
        if ( response == "null" ) {
            response = "";
        }
        res.set_content(response, "text/plain");
    });

    svr.Get(R"(/api/v2/data/(.*))", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        const std::string dataHash = req.matches[1];
        std::string dataTitle;
        std::ostringstream oss; oss << "data/" << dataHash << ".txt";
        std::string dataFile = oss.str();
        std::ifstream inFile;
        inFile.open(dataFile);
        getline(inFile, dataTitle);
        inFile.close();
        res.set_content(dataTitle, "text/plain");
    });

    svr.Get(R"(/api/v2/filter/(.*))", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        const std::string prefix = req.matches[1];
        std::vector<std::string> responseList;
        std::string responsePrefix;

        json mined;
        try {
            std::ifstream i("hashes/hashMap.json");
            mined = json::parse(i);
        } catch (json::parse_error& ex){}

        for (auto it = mined.begin(); it != mined.end(); ++it) {
            for (std::string s: it.value()) {
                responsePrefix = s.substr(0, prefix.length());
                if  ( responsePrefix == prefix ) {
                    responseList.push_back(s);
                }
            }
        }

        std::string response;
        for (std::string s: responseList) {
            response.append(s);
            response.append("\n");
        }

        res.set_content(response, "text/plain");
    });

    //MAIN POST FUNCTION
    svr.Post("/api/v2/mine", [&](const Request &req, Response &res, const ContentReader &content_reader) {
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<long long unsigned> distribution(0,0xFFFFFFFFFFFFFFFF);
        uint64_t nonceNum = distribution(generator);
        res.set_header("Access-Control-Allow-Origin", "*");
        std::cout << "Recieved Mining    Request" << std::endl;

        if (req.has_param("source")) {
            sourceHashStr = req.get_param_value("source");
        }
        if (req.has_param("data")) {
            mineData = req.get_param_value("data");
        }
        if (req.has_param("target")) {
            target = req.get_param_value("target");
        }
        std::cout << "Recieved Source, Data and Target" << std::endl;
        
        miner(nonceNum);
        /*auto Miner1 = taskflow.emplace([&](){miner(nonceNum);}).name("Miner 1");
        nonceNum = nonceNum + 1;
        auto Miner2 = taskflow.emplace([&](){miner(nonceNum);}).name("Miner 2");
        nonceNum = nonceNum + 1;
        auto Miner3 = taskflow.emplace([&](){miner(nonceNum);}).name("Miner 3");
        nonceNum = nonceNum + 1;
        auto Miner4 = taskflow.emplace([&](){miner(nonceNum);}).name("Miner 4");
        executor.run(taskflow).wait();*/
    });
    //PORT FOR SERVER TO LISTEN ON
    svr.listen("0.0.0.0", 8680);
}