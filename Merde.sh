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
echo "    test de tronquage"
echo "--------------------------------------------------------------------"
./ufs truncate /mauvais.txt 5
./ufs truncate /doc 6
./ufs truncate /b.txt 5
./ufs read /b.txt 0 10
echo "On doit libérer un bloc de données pour la prochaine opération"
./ufs truncate /b.txt 0

