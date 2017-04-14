#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png

echo 
echo 
echo "--------------------------------------------------------------------"
echo "            Tester la fonction rename sur répertoire"
echo "--------------------------------------------------------------------"
./ufs ls /Bonjour
./ufs ls /doc
./ufs rename /doc/tmp /Bonjour/tmpmv


./ufs ls /Bonjour
./ufs ls /doc



echo -e "\nOn vérifie que le nombre de lien pour /Bonjour augmente de 1 et qu'il diminue de 1 pour /doc:"
./ufs ls /
echo -e "\nOn vérifie que le sous-réperoire tmpmv contient encore subtmp et new.txt:"
./ufs ls /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien vers ce même répertoire n'augmente pas si on répète l'opération:"
./ufs rename /Bonjour/tmpmv /Bonjour/tmpmv2
./ufs rename /Bonjour/tmpmv2 /Bonjour/tmpmv3
./ufs ls /Bonjour



echo -e "affichage de bonjour et doc"
./ufs ls /Bonjour
./ufs ls /doc


echo -e "########################################################"

cho 
echo
echo "--------------------------------------------------------------------"
echo "          Tester la fonction rename sur fichier ordinaire"
echo "--------------------------------------------------------------------"
./ufs rename /Bonjour/LesAmis.txt /Bonjour/OncleG.txt
./ufs ls /Bonjour
./ufs rename /Bonjour/OncleG.txt /DansRoot.txt
./ufs ls /Bonjour
./ufs ls /

