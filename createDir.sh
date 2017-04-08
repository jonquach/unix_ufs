#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png


echo
echo
echo "--------------------------------------------------------------------"
echo "                Tester la création d'un répertoire"
echo "--------------------------------------------------------------------"
./ufs blockfree; N_FREEBLOCK=$?;
./ufs ls /Bonjour
./ufs mkdir /Bonjour/newdir
let "N_FREEBLOCK=$N_FREEBLOCK-1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a utilisé un bloc:"
./ufs blockfree
echo -e "\nOn vérifie que le nombre de lien nlink pour /Bonjour augmente de 1, à cause du sous-répertoire newdir:"
./ufs ls /Bonjour
./ufs ls /

