#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png


./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande rmdir"
echo "--------------------------------------------------------------------"
./ufs blockfree; N_FREEBLOCK=$?;
./ufs rmdir /rep
./ufs ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a été libéré:"
./ufs blockfree
echo -e "\nDoit échouer avec -3, car /doc n'est pas vide:."
./ufs rmdir /doc
./ufs ls /
echo -e "\nDoit échouer avec -3, car /doc/tmp n'est pas vide:"
./ufs rmdir /doc/tmp

echo -e "\n------------------"
./ufs rmdir /doc/tmp/subtmp/b.txt
