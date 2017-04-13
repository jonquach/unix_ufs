#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png

# echo
# echo "--------------------------------------------------------------------"
# echo "                     montrer le contenu du disque"
# echo "--------------------------------------------------------------------"
# ./ufs ls /
# ./ufs ls /doc
# # ./ufs ls /doc/tmp
# # ./ufs ls /doc/tmp/subtmp
# # ./ufs ls /rep
# # ./ufs ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande symlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs libre DOIT changer"
./ufs blockfree; N_FREEBLOCK=$?;
echo -e "\nDoit réussir:"
./ufs symlink /b.txt /symlinkb.txt
echo -e "\nDoit échouer avec -2, car symlinkb.txt existe déjà:"
./ufs symlink /b.txt /symlinkb.txt
let "N_FREEBLOCK=$N_FREEBLOCK-1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
./ufs blockfree
echo -e "\nDoit afficher des numéros d'i-node différents pour /b.txt et /symlinkb.txt:"
./ufs ls /
