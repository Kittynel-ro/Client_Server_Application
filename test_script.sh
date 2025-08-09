for i in {1..99}
do
    echo mesaj$i | ./build/client marian$i password &
done