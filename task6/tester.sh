echo "====== cleaning after previous test ======"
echo ""
rm "my_disc"
rm "test1.txt"
rm "test2.txt"
rm "test3.txt"
touch "test1.txt"
touch "test2.txt"
touch "test3.txt"
cp blueprint1.txt "test1.txt"
cp blueprint2.txt "test2.txt"
cp blueprint3.txt "test3.txt"

echo "====== create disc ======"
echo ""
./main create_disc "my_disc" 70000
echo "====== print files stored on disc ======"
echo ""
./main ls "my_disc"
echo "====== print taken space ======"
echo ""
./main print_taken_space "my_disc"
echo "====== uploading file "test1.txt" ======"
echo ""
./main cp_to_disc "test1.txt" "my_disc"
echo "======finished upload, printing files on disc ======"
echo ""
./main ls "my_disc"
echo "====== printing taken disc space ======"
echo ""
./main print_taken_space "my_disc"
echo "====== printing disc map ======"
echo ""
./main print_memory_map "my_disc"

echo "====== uploading two file of size around 5kb - two blocks ======"
echo ""
./main cp_to_disc "test2.txt" "my_disc"

echo "====== finished upload, printing files on disc ======"
echo ""
./main ls "my_disc"

echo "====== printing taken disc space ======"
echo ""
./main print_taken_space "my_disc"

echo "====== printing disc map ======"
echo ""
./main print_memory_map "my_disc"

echo "====== uploading file of size around 10kb - four blocks ======"
echo ""
./main cp_to_disc "test3.txt" "my_disc"

echo "====== finished upload, printing files on disc ======"
echo ""
./main ls "my_disc"

echo "====== printing taken disc space ======"
echo ""
./main print_taken_space "my_disc"

echo "====== printing disc map ======"
echo ""
./main print_memory_map "my_disc"

echo "====== removing file "test2.txt" from local storage ======"
echo ""
rm "test2.txt"

echo "====== finished removal, copyting same file from disc ======"
echo ""
./main cp_from_disc  "test2.txt" "my_disc"

echo "====== printing fetched file ======"
echo ""
cat test2.txt
echo ""

echo "====== removing file "test3.txt" from disc ======"
echo ""
./main rm "test3.txt" my_disc

echo "====== finished removal, printing files on disc ======"
echo ""
./main ls "my_disc"

echo "====== printing taken disc space ======"
echo ""
./main print_taken_space "my_disc"

echo "====== printing disc map ======"
echo ""
./main print_memory_map "my_disc"