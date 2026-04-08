i=0;\
while true;\
do ((i++));\
make;\
echo "********************************** Test #${i} *************************************";\
./main;\
read -p "Press Enter to Continue ...";\
done
