#/bin/sh

[ "$#" -ne "1" ] && echo "please provide binary" && exit 1

tmp=$(mktemp)
tmp2=$(mktemp)

count=0
cd $(dirname $1)
echo "$(realpath $1)" > $tmp

while [ "$count" -ne "$(cat $tmp | wc -l)" ]; do
   for i in $(cat $tmp); do
      ldd $i | grep "lib/" | grep "\.so\." | sed "s#[ ]*.*=> \(.*/lib/.*\) (.*)#\1#g" >> $tmp;
   done
   cat $tmp | sort | uniq > $tmp2 && cat $tmp2 > $tmp
   count=$(cat $tmp | wc -l)
done

for i in $(cat $tmp | grep "lib/"); do
   cp $i .
done
rm -f $tmp $tmp2
