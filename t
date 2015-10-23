make clean
make
make test$1
./test$1 > output/test$1.out
echo diffing
diff testResults/test$1.txt output/test$1.out
echo after diff
