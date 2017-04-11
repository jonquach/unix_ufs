#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
./ufs ls /
./ufs ls /doc
# ./ufs ls /doc/tmp
# ./ufs ls /doc/tmp/subtmp
# ./ufs ls /rep
# ./ufs ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
./ufs create /doc/new.txt
./ufs ls /doc

echo

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
echo "                    Tester la commande hardlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs libre ne doit pas changer"
./ufs blockfree; N_FREEBLOCK=$?;
echo -e "\nDoit réussir:"
./ufs hardlink /doc/new.txt /doc/hlnb.txt
echo -e "\nDoit échouer avec -2, car hlnb.txt existe déjà:"
./ufs hardlink /doc/new.txt /doc/hlnb.txt
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
./ufs blockfree
echo -e "\nDoit afficher les mêmes numéros d'i-node pour /b.txt et /hlnb.txt:"
./ufs ls /doc

echo
echo
echo "--------------------------------------------------------------------"
echo "                          tests de lecture"
echo "--------------------------------------------------------------------"

./ufs read /hlnb.txt 0 30
