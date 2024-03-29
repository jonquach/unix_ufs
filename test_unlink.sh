#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande hardlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs libre ne doit pas changer"
./ufs blockfree; N_FREEBLOCK=$?;
echo -e "\nDoit réussir:"
./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit échouer avec -2, car hlnb.txt existe déjà:"
./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit échouer avec -3, car /doc est un repertoire:"
./ufs hardlink /doc /hlb.txt
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
./ufs blockfree
echo -e "\nDoit afficher les mêmes numéros d'i-node pour /b.txt et /hlnb.txt:"
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande unlink"
echo "--------------------------------------------------------------------"
./ufs unlink /b.txt
./ufs ls /
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode est toujours détenu par hlnb.txt:"
./ufs blockfree
./ufs unlink /hlnb.txt
./ufs ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode a été libéré:"
./ufs blockfree
echo -e "\nDoit échouer avec -1, car /b.txt n'existe plus:"
./ufs unlink /b.txt
echo -e "\nDoit échouer avec -1, car /doc/tmp/b.txt n'existe pas:"
./ufs unlink /doc/tmp/b.txt 
./ufs unlink /doc/tmp/subtmp/b.txt
./ufs ls /doc/tmp/subtmp
echo -e "\nDoit échouer avec -2, car /doc est un répertoire:"
./ufs unlink /doc
