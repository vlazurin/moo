# moo
Operating system for education purposes
## How to compile system (Ubuntu)
Install cmake
```bash
sudo apt-get install software-properties-common
sudo add-apt-repository ppa:george-edison55/cmake-3.x
sudo apt-get update
sudo apt-get install cmake
```
Compile system
```bash
mkdir ./build
cd ./build
cmake ../src
make
```
## How to run system in VirtualBox
```bash
./create_vm.sh
./run.sh
```
