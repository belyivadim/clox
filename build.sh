if [ "$1" == "rebuild" ]; then
  rm -rf build
  mkdir build
elif [ "$1" == "init" ]; then
  mkdir build
fi 

cd build
cmake ..
make
cd ..
