#To indent with opening parenthesis, or to fix line over 80 characters,
# use gedit to edit the source code. Don't edit the patch file. It does not work. Netbeans also does not work. 
git checkout -b tmpbranch
git commit -a -m "Poptrie based routing table lookup"
git format-patch -1 HEAD

