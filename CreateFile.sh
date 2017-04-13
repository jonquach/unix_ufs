#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png


echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
./ufs create /Doge.wow
./ufs ls /
./ufs create /doc/tmp/new.txt 
./ufs ls /
./ufs ls /doc/tmp
