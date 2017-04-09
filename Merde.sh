#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
./ufs ls /
# ./ufs ls /doc
# ./ufs ls /doc/tmp
# ./ufs ls /doc/tmp/subtmp
# ./ufs ls /rep
# ./ufs ls /Bonjour

# echo
# echo
# echo "--------------------------------------------------------------------"
# echo "                    Tester la commande readlink"
# echo "--------------------------------------------------------------------"
# ./ufs readlink /slnb.txt
# echo -e "\nDoit échouer avec -1, car hlnb.txt n'est pas un lien symbolique:"
# ./ufs readlink /hlnb.txt

# echo
# echo
# echo "--------------------------------------------------------------------"
# echo "                          tests de lecture"
# echo "--------------------------------------------------------------------"

# ./ufs read /doc/tmp/subtmp/b.txt 0 0
# ./ufs read /b.txt 0 30
#./ufs read /b.txt 5 10
#./ufs read /b.txt 10 30
#./ufs read /b.txt 10 5

# echo
# echo
# echo "--------------------------------------------------------------------"
# echo "                Tester la création d'un répertoire"
# echo "--------------------------------------------------------------------"
# ./ufs blockfree; N_FREEBLOCK=$?;
# ./ufs ls /Bonjour
# ./ufs mkdir /Bonjour/newdir
# let "N_FREEBLOCK=$N_FREEBLOCK-1"
# echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a utilisé un bloc:"
# ./ufs blockfree
# echo -e "\nOn vérifie que le nombre de lien nlink pour /Bonjour augmente de 1, à cause du sous-répertoire newdir:"
# ./ufs ls /Bonjour
# ./ufs ls /


echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
./ufs create /Doge.wow
./ufs ls /
# ./ufs create /doc/tmp/new.txt 
# ./ufs ls /
# ./ufs ls /doc/tmp
