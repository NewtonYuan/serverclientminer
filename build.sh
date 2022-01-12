echo "building server"
g++ -std=c++17 server.cpp -I taskflow/taskflow -I PicoSHA2 -O2 -pthread -o server
echo "server compiled. Running"
./server