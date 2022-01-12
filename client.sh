echo "building client"
g++ -std=c++17 client.cpp -pthread -o client
echo "client compiled. Running"
./client

