if [[ ! -d "build_ut/" ]]
then
    mkdir build_ut
fi

cd build_ut

cmake .. -DCMAKE_BUILD_TYPE=Debug -DUNIT_TESTS=On
echo "=============================================="
echo "=== Project configured correctly, building ==="
echo "=============================================="

cmake --build .
if [[ $? -eq 0 ]];
then
   echo "=========================="
   echo "=== Build successfully ==="
   echo "=========================="
else
   echo "=========================="
   echo "====== Build error! ======"
   echo "=========================="
   exit 1
fi

ctest --output-on-failure --schedule-random --timeout 60
