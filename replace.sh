for f in experiment2/*
do
    sed -i '6s/0/6/' $f
    sed -i '5s/0/4/' $f
    sed -i '4s/0/1024/' $f
done
