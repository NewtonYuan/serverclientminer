echo "building test server"
g++ -std=c++17 testserver.cpp -I taskflow/taskflow -I PicoSHA2 -O2 -pthread -o testserver
echo "test server compiled. Running"
./testserver