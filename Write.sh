# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier google-go.png.orig vers google-go.png" 
cp google-go.png.orig google-go.png

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
./ufs ls /
./ufs ls /doc
./ufs ls /doc/tmp
./ufs ls /doc/tmp/subtmp
./ufs ls /rep
./ufs ls /Bonjour

echo -e "Le contenu du fichier avant : "
./ufs read /b.txt 0 25


echo
echo
echo "--------------------------------------------------------------------"
echo "                1  test d'ecriture de 40 caracteres"
echo "--------------------------------------------------------------------"
./ufs blockfree
./ufs write /b.txt "1234567890ABCDEFGHIJ1234567890ABCDEFGHIJ" 0 
./ufs stat /b.txt
./ufs blockfree

echo -e "Le contenu du fichier apres 1 : "
./ufs read /b.txt 0 50


echo
echo
echo "--------------------------------------------------------------------"
echo "      2 test d'ecriture de 1 caracteres en milieu de fichier"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "-" 14 
./ufs stat /b.txt
./ufs blockfree  
./ufs read /b.txt 0 20


echo -e "Le contenu du fichier apres 2 : "
./ufs read /b.txt 0 50



echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture de 1 caracteres, mais trop loin"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "X" 41 
./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "   test d'ecriture exactement après le dernier caractère du fichier"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "+" 40 
./ufs stat /b.txt
./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture augmentant la taille du fichier, mais sans saisie de nouveau bloc"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "abcdefghij" 40 
./ufs stat /b.txt
./ufs blockfree; N_FREEBLOCK=$?;  
./ufs read /b.txt 0 60



echo
echo
echo "--------------------------------------------------------------------"
echo "  test d'ecriture qui doit provoquer la saisie de 2 nouveaux blocs"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "abcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJ" 0 
./ufs stat /b.txt
let "N_FREEBLOCK=$N_FREEBLOCK-2"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
./ufs blockfree  

echo
echo
echo "-----------------------------------------------------------------------"
echo "  test de lecture dans le fichier plus gros, qui chevauche deux blocs"
echo "-----------------------------------------------------------------------"
./ufs read /b.txt 500 30
