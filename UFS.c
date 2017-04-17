#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

// Quelques fonctions qui pourraient vous être utiles
int NumberofDirEntry(int Size) {
	return Size/sizeof(DirEntry);
}

int min(int a, int b) {
	return a<b ? a : b;
}

int max(int a, int b) {
	return a>b ? a : b;
}

/* Cette fonction va extraire le repertoire d'une chemin d'acces complet, et le copier
   dans pDir.  Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pDir le string "/doc/tmp" . Si le chemin fourni est pPath="/a.txt", la fonction
   va retourner pDir="/". Si le string fourni est pPath="/", cette fonction va retourner pDir="/".
   Cette fonction est calquée sur dirname, que je ne conseille pas d'utiliser car elle fait appel
   à des variables statiques/modifie le string entrant. Voir plus bas pour un exemple d'utilisation. */
int GetDirFromPath(const char *pPath, char *pDir) {
	strcpy(pDir,pPath);
	int len = strlen(pDir); // length, EXCLUDING null
	int index;

	// On va a reculons, de la fin au debut
	while (pDir[len]!='/') {
		len--;
		if (len <0) {
			// Il n'y avait pas de slash dans le pathname
			return 0;
		}
	}
	if (len==0) {
		// Le fichier se trouve dans le root!
		pDir[0] = '/';
		pDir[1] = 0;
	}
	else {
		// On remplace le slash par une fin de chaine de caractere
		pDir[len] = '\0';
	}
	return 1;
}

/* Cette fonction va extraire le nom de fichier d'une chemin d'acces complet.
   Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pFilename le string "a.txt" . La fonction retourne 1 si elle
   a trouvée le nom de fichier avec succes, et 0 autrement. Voir plus bas pour
   un exemple d'utilisation. */
int GetFilenameFromPath(const char *pPath, char *pFilename) {
	// Pour extraire le nom de fichier d'un path complet
	char *pStrippedFilename = strrchr(pPath,'/');
	if (pStrippedFilename!=NULL) {
		++pStrippedFilename; // On avance pour passer le slash
		if ((*pStrippedFilename) != '\0') {
			// On copie le nom de fichier trouve
			strcpy(pFilename, pStrippedFilename);
			return 1;
		}
	}
	return 0;
}


/* Un exemple d'utilisation des deux fonctions ci-dessus :
int bd_create(const char *pFilename) {
	char StringDir[256];
	char StringFilename[256];
	if (GetDirFromPath(pFilename, StringDir)==0) return 0;
	GetFilenameFromPath(pFilename, StringFilename);
	                  ...
*/


/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode) {
	printf("\t\t========= inode %d ===========\n",iNode.iNodeStat.st_ino);
	printf("\t\t  blocks:%d\n",iNode.iNodeStat.st_blocks);
	printf("\t\t  size:%d\n",iNode.iNodeStat.st_size);
	printf("\t\t  mode:0x%x\n",iNode.iNodeStat.st_mode);
	int index = 0;
	for (index =0; index < N_BLOCK_PER_INODE; index++) {
		printf("\t\t      Block[%d]=%d\n",index,iNode.Block[index]);
	}
}


/* ----------------------------------------------------------------------------------------
					            à vous de jouer, maintenant!
   ---------------------------------------------------------------------------------------- */

int getInodeBlockNumAndPos(ino iNodeNum, int *iNodeBlockNum, int *iNodePosition);
int getInodeEntry(ino iNodeNum, iNodeEntry *inodeEntry);
int isFolder(iNodeEntry iNodeStuff);
int getLeftPart(const char *path, char **leftPart);
ino getInodeNumberFromPath(ino inode, const char *pathToFind);
int findFirstFreeInode(char freeInodes[BLOCK_SIZE]);
int getFreeInode();
int findFirstFreeBlock(char freeInodes[BLOCK_SIZE]);
int getFreeBlock();
int releaseFreeInode(ino inodeNumber);
int ReleaseFreeBlock(ino BlockNum);
void updateDir(iNodeEntry * destDirInode, ino inodeNum, int inc, char *filename);
void updateInode(iNodeEntry *ine);

/*Retourne le numéro de block et position d'une inode*/
int getInodeBlockNumAndPos(ino iNodeNum, int *iNodeBlockNum, int *iNodePosition)
{
  if (iNodeNum > N_INODE_ON_DISK || iNodeNum < 0)
    return -1;

  if (iNodeNum >= 0 && iNodeNum <= 15)
  {
    *iNodeBlockNum = 4;
    *iNodePosition = iNodeNum;
  }
  else if (iNodeNum >= 16 && iNodeNum <= 31)
  {
    *iNodeBlockNum = 5;
    //on fait la difference pour partir de 0 dans le bloc 2
    *iNodePosition = NUM_INODE_PER_BLOCK - iNodeNum;
  }
  return (0);
}

/*Recupere le inodeEntry en utilisant un numéro d'inode*/
int getInodeEntry(ino iNodeNum, iNodeEntry *inodeEntry)
{
  int iNodeBlockNum = -1;
  int iNodePosition = -1;
  iNodeEntry *inodesBlock = NULL;
  char blockData[BLOCK_SIZE];


  if (iNodeNum > N_INODE_ON_DISK || iNodeNum < 0)
    return (-1);

  if (iNodeNum >= 0 && iNodeNum <= 15)
    {
      iNodeBlockNum = 4;
      iNodePosition = iNodeNum;
    }
  else if (iNodeNum >= 16 && iNodeNum <= 31)
    {
      iNodeBlockNum = 5;
      iNodePosition = NUM_INODE_PER_BLOCK - iNodeNum; //on fait la difference pour partir de 0 dans le bloc 2
    }
  else
    return (-1);

  ReadBlock(iNodeBlockNum, blockData);
  inodesBlock = (iNodeEntry *) blockData;
  *inodeEntry = inodesBlock[iNodePosition];
  return 0;
}

/*Verifie si l'inode est un dossier*/
int isFolder(iNodeEntry iNodeStuff)
{
  if (iNodeStuff.iNodeStat.st_mode & G_IFDIR)
    return 1;
  else
    return 0;
}

/*Retourne la partie gauche de slash en slash du path*/ 
int getLeftPart(const char *path, char **leftPart)
{
  if (strlen(path) == 0)
    {
      return -1;
    }
  if (path[0] == '/')//si ca commence par un / alors on le dégage
    path = path + 1;

  int i = 0;
  int ct = 0;
  char *newString = NULL;

  while (path[i] != '\0' && path[i] != '/')
    {
      ct++;
      i++;
    }
  newString = malloc((ct + 1) * sizeof(char));
  i = 0;
  while (path[0] != '\0' && path[0] != '/')
    {
      newString[i] = path[0];
      path++;//on incremente le pointeur du coup on découpe la chaine petit a petit (et apres avoir fait la copie du char dans newString)
      i++;
    }
  newString[i] = '\0';
  *leftPart = newString;
  return (i + 1);
}

/*Retourne le numéro d'inode d'un fichier par son path*/
//au debut ROOT_INODE
ino getInodeNumberFromPath(ino inode, const char *pathToFind)
{
  iNodeEntry iNodeEntry;
  char fileDataBlock[BLOCK_SIZE];

  if (strcmp(pathToFind, "/") == 0)
    return ROOT_INODE;
  getInodeEntry(inode, &iNodeEntry);

  if (isFolder(iNodeEntry) == 1)
    {

      char *leftPathPart = NULL;
      int ret = getLeftPart(pathToFind, &leftPathPart);


      if (ret == -1) //on est arrivé au bout du path.
	{
	  return iNodeEntry.iNodeStat.st_ino;
	}

      pathToFind += ret;//on increment le ptr ici car dans getLeftPart ca fonctionne pas...

      ReadBlock(iNodeEntry.Block[0], fileDataBlock);
      DirEntry *dirEntry = (DirEntry *)fileDataBlock;
      int nbFile = iNodeEntry.iNodeStat.st_size / sizeof(DirEntry);
      int i = 0;
      while(i < nbFile)
	{
	  if (strcmp(dirEntry[i].Filename, leftPathPart) == 0)//si le nom de dossier est le meme que le nom de path suivant alors on peut choper son inode et recommencer
	    {
     	      return getInodeNumberFromPath(dirEntry[i].iNode, pathToFind); //dans la structure y a l'inode a coté du filename
	    }
	  i++;
	}
      // NOT FOUND
    if (strlen(pathToFind) != 0) {
      // Folder does not exist
      return -1;
    }
    // File does not exist
    return -2;
    }
  else
    {
      return iNodeEntry.iNodeStat.st_ino;
    }
}

/*Retourne la 1ere inode libre trouvée*/
int findFirstFreeInode(char freeInodes[BLOCK_SIZE])
{
  int curInode = ROOT_INODE;

  while (curInode < N_INODE_ON_DISK && freeInodes[curInode] == 0) {
    curInode++;
  }

  if (curInode >= N_INODE_ON_DISK) {
    return (-1);
  }
  return curInode;
}

/*Saisie la 1ere inode libre trouvée*/
int getFreeInode()
{
  char freeInodesBlock[BLOCK_SIZE];
  int freeInode = -1;

  ReadBlock(FREE_INODE_BITMAP, freeInodesBlock);
  freeInode = findFirstFreeInode(freeInodesBlock);

  if (freeInode != -1) {
    freeInodesBlock[freeInode] = 0;
    printf("GLOFS: Saisie i-node %d\n", freeInode);
    WriteBlock(FREE_INODE_BITMAP, freeInodesBlock);
    return freeInode;
  }
  return -1;
}

/*Retourne le 1er block libre*/
int findFirstFreeBlock(char freeInodes[BLOCK_SIZE])
{
  int curInode = ROOT_INODE;

  while (curInode < N_INODE_ON_DISK && freeInodes[curInode] == 0) {
    curInode++;
  }

  if (curInode >= N_INODE_ON_DISK) {
    return -1;
  }
  return curInode;
}

/*Saisie le 1er block libre trouvé*/
int getFreeBlock()
{
  char dataBlock[BLOCK_SIZE];
  int blockNum = 6;

  ReadBlock(FREE_BLOCK_BITMAP, dataBlock);

  while (dataBlock[blockNum] == 0 && blockNum < N_BLOCK_ON_DISK) {
    blockNum++;
  }

  if (blockNum < N_BLOCK_ON_DISK) {
    dataBlock[blockNum] = 0;
    printf("GLOFS: Saisie bloc %d\n", blockNum);
    WriteBlock(FREE_BLOCK_BITMAP, dataBlock);
    return blockNum;
  }
  return -1;
}

/*Relache une inode*/
int releaseFreeInode(ino inodeNumber)
{
  char blockData[BLOCK_SIZE];

  ReadBlock(FREE_INODE_BITMAP, blockData);
  blockData[inodeNumber] = 1;

  WriteBlock(FREE_INODE_BITMAP, blockData);
  printf("GLOFS: Relache i-node %d\n", inodeNumber);
  return (1);
}

/*Relache un block*/
int ReleaseFreeBlock(ino BlockNum)
{
  char BlockFreeBitmap[BLOCK_SIZE];

  ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
  BlockFreeBitmap[BlockNum] = 1;
  printf("GLOFS: Relache bloc %d\n", BlockNum);
  WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
  return (1);
}

/*Cette fonction retourne le nombre de bloc de données li*/
int bd_countfreeblocks(void)
{
  int nbFreeBlocks;
  char freeBlock[BLOCK_SIZE];

  ReadBlock(FREE_BLOCK_BITMAP, freeBlock);

  for (int i = 0; i < N_BLOCK_ON_DISK; ++i) {
    if (freeBlock[i] != 0)
      nbFreeBlocks++;
  }

  return nbFreeBlocks;
}

/*Cette  fonction  copie  les  métadonnées gstat du fichier pFilename vers  le  pointeur pStat.
Les métadonnées du fichier pFilename doivent demeurer inchangées.
La fonction retourne -1 si le fichier pFilename est inexistant. Autrement, la fonction retourne 0.
*/
int bd_stat(const char *pFilename, gstat *pStat)
{
  ino iNodeNum;
  iNodeEntry iNodeEntry;

  iNodeNum = getInodeNumberFromPath(ROOT_INODE, pFilename);

  if (iNodeNum == -2 || iNodeNum == -1)
    return -1;

  getInodeEntry(iNodeNum, &iNodeEntry);
  *pStat = iNodeEntry.iNodeStat;

  return 0;
}

/*
  Create a empty file, with rwx permision at the pointed path
  Return: 0 on success, -1 if directory does not exist, -2 if file alread exist
*/
int bd_create(const char *pFilename)
{
  char filename[FILENAME_SIZE];
  char dirname[BLOCK_SIZE];
  iNodeEntry iNodeEntry, iNodeDir;
  ino iNodeNum;
  ino dirInode = 0;

  iNodeNum = getInodeNumberFromPath(ROOT_INODE, pFilename);

  if (GetDirFromPath(pFilename, dirname) == 0)
    return (-1);
  if (GetFilenameFromPath(pFilename, filename) == 0)
    return (-1);

  if (iNodeNum == -1)
    return -1; // Directory does not exist
  if (iNodeNum != -2)
    return -2; // File already exist

  iNodeNum = getFreeInode();

  getInodeEntry(iNodeNum, &iNodeEntry);
  iNodeEntry.iNodeStat.st_ino = iNodeNum;
  iNodeEntry.iNodeStat.st_mode = G_IFREG | G_IRWXU | G_IRWXG;
  iNodeEntry.iNodeStat.st_nlink = 1;
  iNodeEntry.iNodeStat.st_size = 0;
  iNodeEntry.iNodeStat.st_blocks = 0;

  dirInode = getInodeNumberFromPath(ROOT_INODE, dirname);

  getInodeEntry(dirInode, &iNodeDir);

  updateDir(&iNodeDir, iNodeEntry.iNodeStat.st_ino, 0, filename);
  updateInode(&iNodeEntry);

  return 0;
}

/*
Cette fonction va lire numbytes octets dans le fichier pFilename, à partir de la position offset,
et les copier dans buffer. La valeur retournée par la fonction est le nombre d’octets lus.
Par exemple, si le fichier fait 100 octets et que vous faites une lecture pour offset=10 et 
numbytes=200, la valeur retournée par bd_read sera 90. Si le fichier pFilename est inexistant,
la fonction devra retourner -1. Si le fichier pFilename est un répertoire,
la fonction devra retourner -2. Si l’offset fait en sorte qu’il dépasse la taille du fichier,
cette fonction devra simplement retourner 0, car vous ne pouvez pas lire aucun caractère.
Notez que le nombre de blocs par fichier est limité à 1, ce qui devrait simplifier le code de lecture. 
*/
int bd_read(const char *pFilename, char *buffer, int offset, int numbytes)
{
  ino inodeNum;
  if ((inodeNum = getInodeNumberFromPath(ROOT_INODE, pFilename)) < 0)
    return (-1);
  iNodeEntry iNodeEntry;
  char fileData[BLOCK_SIZE];
  int ctRead = 0;
  int i = offset;

  getInodeEntry(inodeNum, &iNodeEntry);
  if (isFolder(iNodeEntry) == 1)
    return (-2);
  ReadBlock(iNodeEntry.Block[0], fileData);

  while (i < (offset + numbytes) && i < iNodeEntry.iNodeStat.st_size)
    {
      buffer[ctRead] = fileData[i];
      ctRead++;
      i++;
    }
  buffer[ctRead] = '\0';

  return ctRead;
}

/*
Cette fonction doit créer le répertoire pDirName. Si le chemin d’accès à pDirName est inexistant, ne faites 
rien et retournez -1, par exemple si on demande de créer le répertoire /doc/tmp/totoet que le répertoire 
/doc/tmp n’existe  pas.  Assurez-vous  que  l’i-node correspondant  au  répertoire  est  marqué  comme 
répertoire (bit G_IFDIR de st_mode à 1). Si le répertoire pDirName existe déjà, retournez avec -2. 
Pour les permissions rxw, simplement les mettre toutes à 1, en faisant st_mode|=G_IRWXU|G_IRWXG. 
Assurez-vous aussi que le répertoire contiennent les deux répertoires suivants : « . » et « .. ».
N’oubliez-pas d’incrémenter st_nlink pour le répertoire actuel « . » et parent « .. ».
En cas de succès, retournez 0. 
*/
int bd_mkdir(const char *pDirName)
{
  char pathRight[500];
  char pathLeft[500];
  ino iNodeNumLeft;
  iNodeEntry iNodeEntryLeft;
  ino iNodeNumRight;
  iNodeEntry iNodeEntryRight;

  if (getInodeNumberFromPath(ROOT_INODE, pDirName) >= 0)
    return (-2);
  
  //Découpage du path en left et right
  if (GetDirFromPath(pDirName, pathLeft) == 0)
    return (-1);

  if (GetFilenameFromPath(pDirName, pathRight) == 0)
    return (-1);

  //Récupération du numéro d'inode de left
  if ((iNodeNumLeft = getInodeNumberFromPath(ROOT_INODE, pathLeft)) == -1)
    return (-1);

  //Récupération de l'entry pour left
  getInodeEntry(iNodeNumLeft, &iNodeEntryLeft);

  //check que left est bien un dossier !
  if (isFolder(iNodeEntryLeft) != 1)//LNK aussi ?
    return (-1);

  //on récupère un numéro d'inode libre (pour right)
  ino freeInodeNum = getFreeInode();
  int freeBlockNum = getFreeBlock();
  iNodeEntry freeInodeEntry;

  //maj du dossier parent
  updateDir(&iNodeEntryLeft, freeInodeNum, 1, pathRight);

  //Recupération du inode stuff de l'inode libre. (celle du nouveau dossier a ajouter)
  getInodeEntry(freeInodeNum, &freeInodeEntry);
  //on update avec le nouveau bloc de donnée
  freeInodeEntry.Block[0] = freeBlockNum;
  //on update le numéro de l'inode libre
  freeInodeEntry.iNodeStat.st_ino = freeInodeNum;
  //les flags donné dans le sujet
  freeInodeEntry.iNodeStat.st_mode = G_IFDIR | G_IRWXU | G_IRWXG;
  //on met à 2 pour . et .. et le dossier parent
  freeInodeEntry.iNodeStat.st_nlink = 3;
  //Il contient . et ..
  freeInodeEntry.iNodeStat.st_size = 2 * sizeof(DirEntry);
  //bon bah si j'ai bien compris y a tj que 1 block de toute facon...
  freeInodeEntry.iNodeStat.st_blocks = 1;
  //on save tout ca
  updateInode(&freeInodeEntry);

  //faut ajouter . et .. maintenant
  char innerBlock[BLOCK_SIZE];

  ReadBlock(freeBlockNum, innerBlock);

  DirEntry * dirEntry = (DirEntry *) innerBlock;

  //pointe sur lui meme
  strcpy(dirEntry[0].Filename, ".");
  dirEntry[0].iNode = freeInodeNum;

  //pointe sur le right (dossier contenant)
  strcpy(dirEntry[1].Filename, "..");
  dirEntry[1].iNode = iNodeNumRight;

  //on save le block avant le cast
  WriteBlock(freeBlockNum, innerBlock);
  return (0);
}

/*Ajoute un dossier et met à jours les informations*/
void updateDir(iNodeEntry * destDirInode, ino inodeNum, int inc, char *filename)
{
  char dataBlock[BLOCK_SIZE];
  DirEntry *dirEntry;

  //On incrémente le nombre de link de left (puisqu'on va lui ajouter un fichier à l'interieur)
  if (inc == 1)
    destDirInode->iNodeStat.st_nlink++;

  //on update la size du dossier avec elle meme + un fichier
  destDirInode->iNodeStat.st_size += sizeof(DirEntry);
  //on save
  updateInode(destDirInode);

  //on lit le bloc de donné du dossier (y a les fichiers dedans)
  ReadBlock(destDirInode->Block[0], dataBlock);
  //on le cast en structure de dossier
  dirEntry = (DirEntry *) dataBlock;

  //on déplace le pointeur pour se mettre sur le dernier
  dirEntry += (NumberofDirEntry(destDirInode->iNodeStat.st_size)) - 1;

  //maintenant on maj le contenu du nouveau dirEntry
  dirEntry->iNode = inodeNum;
  strcpy(dirEntry->Filename, filename);

  //et on le save
  WriteBlock(destDirInode->Block[0], dataBlock);
}

/*Met à jour l'inode sur le disque*/
void updateInode(iNodeEntry *ine)
{
  char blockData[BLOCK_SIZE];
  int iNodeBlockNum;
  int iNodePosition;
  iNodeEntry *inodesBlock = NULL;

  if (ine->iNodeStat.st_ino >= 0 && ine->iNodeStat.st_ino <= 15)
    {
      iNodeBlockNum = 4;
      iNodePosition = ine->iNodeStat.st_ino;
    }
  else
    {
      iNodeBlockNum = 5;
      iNodePosition = NUM_INODE_PER_BLOCK - ine->iNodeStat.st_ino;
    }
  ReadBlock(iNodeBlockNum, blockData);
  inodesBlock = (iNodeEntry *) blockData;
  inodesBlock[iNodePosition] = *ine;
  WriteBlock(iNodeBlockNum, blockData);
}

/*
Cette fonction va écrire numbytes octets du buffer dans le fichier 
pFilename, à partir de la position offset(0 indique le début du fichier).
La valeur retournée par la fonction est le nombre d’octets écrits.  
Vous devez vérifier que : 
Le fichier pFilename existe. Dans le cas contraire, cette fonction devra 
retourner -1. 
Le fichier pFilename n’est pas un répertoire.  Dans le cas contraire, cett
e fonction devra retourner -2. 
L’offset doit être plus petit ou égal à la taille du fichier. Dans le cas contraire,
cette fonction devra retourner -3. Par contre, si l’offset est plus grand ou égal à
la taille maximale supportée par ce système de fichier, retournez la valeur -4. 
La taille finale ne doit pas dépasser la taille maximale
d’un fichier sur ce mini-UFS, qui est dicté par le nombre de bloc d’adressage direct 
N_BLOCK_PER_INODE. et la taille d’un bloc. Vous devez 
quand même écrire le plus possible dans le fichier, jusqu’
à atteindre cette taille limite. La fonction 
retournera ce nombre d’octet écrit. 
N’oubliez-pas de modifier la taille du fichier 
st_size
. 
*/
int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes)
{
  iNodeEntry fileInodeEntry;
  int max = offset + numbytes;
  int i = 0;
  int octs = 0;
  int ct = 0;
  ino fileInodeNumber;
  int freeBlock;
  char dataBlock[BLOCK_SIZE];
  char content[BLOCK_SIZE];

  //Si y a rien a ecrire ca sert a rien de continuer
  if (numbytes <= 0)
    return (1);
  
  if ((fileInodeNumber = getInodeNumberFromPath(ROOT_INODE, pFilename)) == -1)
    return (-1);
  getInodeEntry(fileInodeNumber, &fileInodeEntry);
  
  if (isFolder(fileInodeEntry) == 1) //c'est un dossier
    return (-2);
  
  if (offset > fileInodeEntry.iNodeStat.st_size)
    return (-3);

  //soit là soit dans le while.
  if (offset > BLOCK_SIZE)
    return (-4);


  //check si le fichier est vraiment vide
  if (fileInodeEntry.iNodeStat.st_blocks == 0 && fileInodeEntry.iNodeStat.st_size == 0)
    {
      if ((freeBlock = getFreeBlock()) == -1)
	return (0);
      fileInodeEntry.iNodeStat.st_blocks += 1;
      fileInodeEntry.Block[0] = freeBlock;
    }
  
  
  ReadBlock(fileInodeEntry.Block[0], dataBlock);
  

  //on lit le contenu actuel du fichier
  while (i < fileInodeEntry.iNodeStat.st_size)
    {
      content[i] = dataBlock[i];
      i++;
    }

  i = offset;
  while (ct <= numbytes && i < max &&  i <= BLOCK_SIZE)
    {
      content[i] = buffer[ct];
      octs++;
      ct++;
      i++;
    }

  WriteBlock(fileInodeEntry.Block[0] , content);

  if (octs + offset > fileInodeEntry.iNodeStat.st_size)
    {
      fileInodeEntry.iNodeStat.st_size = octs + offset;
    }

  //mettre a jours le contenu maintenant qu'on a fini
  updateInode(&fileInodeEntry);

  return octs;
}

/*
  Cette fonction créer un hardlink entre l’i-node du fichier pPathExistant et le nom de fichier
  pPathNouveauLien. Assurez-vous que le fichier original pPathExistant n’est pas un répertoire (bit
  G_IFDIR de st_mode à 0 et bit G_IFREG à 1), auquel cas retournez -3. Assurez-vous aussi que le
  répertoire qui va contenir le lien spécifié dans pPathNouveauLien existe, sinon retournez -1. N’oubliezpas
  d’incrémenter la valeur du champ st_nlink dans l’i-node. Assurez-vous que la commande
  fonctionne aussi si le lien est créé dans le même répertoire, i.e.
  bd_hardlink("/tmp/a.txt","/tmp/aln.txt")
  Si le fichier pPathNouveauLien, existe déjà, retournez -2. Si le fichier pPathExistant est inexistant,
  retournez -1. Si tout se passe bien, retournez 0.

  Return:
    * 0 on success
    * -1 if file pPathExistant does not exist
    * -2 if file pPathNouveauLien already exist
    * -3 if file pPathExisitant is a directory
*/

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien)
{
  char dirname[BLOCK_SIZE];
  char fhardlink[BLOCK_SIZE];
  iNodeEntry iNodeDir;
  iNodeEntry iNodeEntryExist;
  iNodeEntry iNodeEntryNewFile;
  ino iNodeNumDir;
  ino iNodeNumExist;
  ino iNodeNumNewFile;

  iNodeNumExist = getInodeNumberFromPath(ROOT_INODE, pPathExistant);
  iNodeNumNewFile = getInodeNumberFromPath(ROOT_INODE, pPathNouveauLien);

  if (iNodeNumExist == -1)
    return (-1); // pPathExistant directory does not exist
  if (iNodeNumExist == -2)
    return (-1); // pPathExistant file does not exist
  if (iNodeNumNewFile == -1)
    return (-1); // pPathNouveauLien directory does not exist
  if (iNodeNumNewFile != -2)
    return (-2); // pPathNouveauLien file already exist

  getInodeEntry(iNodeNumExist, &iNodeEntryExist);

  if ((iNodeEntryExist.iNodeStat.st_mode & G_IFDIR) == G_IFDIR)
    return (-3);

  GetDirFromPath(pPathNouveauLien, dirname);
  iNodeNumDir = getInodeNumberFromPath(ROOT_INODE, dirname);

  getInodeEntry(iNodeNumDir, &iNodeDir);

  GetFilenameFromPath(pPathNouveauLien, fhardlink);

  updateDir(&iNodeDir, iNodeEntryExist.iNodeStat.st_ino, 0, fhardlink);
  iNodeEntryExist.iNodeStat.st_nlink++;
  updateInode(&iNodeEntryExist);

  return (0);
}

/*
  Cette fonction sert à retirer un fichier normal (G_IFREG5 à 1) du répertoire dans lequel il est contenu. Le
  retrait se fait en décrémentant de 1 le nombre de lien (st_nlink) dans l’i-node du fichier pFilename et
  en détruisant l’entrée dans le fichier répertoire dans lequel pFilename se situe. Si st_nlink tombe à
  zéro, vous devrez libérer cet i-node et ses blocs de données associés. Si après bd_unlink le nombre de
  lien n’est pas zéro, vous ne pouvez pas libérer l’ i-node, puisqu’il est utilisé ailleurs (via un hardlink).
  N’oubliez-pas de compacter les entrées dans le tableau de DirEntry du répertoire, si le fichier détruit
  n’est pas à la fin de ce tableau. Si pFilename n’existe pas retournez -1. S’il n’est pas un fichier régulier
  G_IFREG, retournez -2. Autrement, retourner 0 pour indiquer le succès.

  Return:
    * 0 on success
    * -1 if pFilename does not exist
    * -2 if pFilename is not a file
*/

int bd_unlink(const char *pFilename)
{
  ino iNodeNumDir;
  ino iNodeNumFile;
  char dirname[BLOCK_SIZE];
  char filename[BLOCK_SIZE];
  char blockData[BLOCK_SIZE];
  iNodeEntry iNodeEntryDir;
  iNodeEntry iNodeEntryFile;

  GetDirFromPath(pFilename, dirname);
  GetFilenameFromPath(pFilename, filename);

  iNodeNumDir = getInodeNumberFromPath(ROOT_INODE, dirname);
  iNodeNumFile = getInodeNumberFromPath(ROOT_INODE, pFilename);

  if (iNodeNumFile == -1)
    return (-1); // pPathExistant directory does not exist
  if (iNodeNumFile == -2)
    return (-1); // pPathExistant file does not exist

  getInodeEntry(iNodeNumDir, &iNodeEntryDir);
  getInodeEntry(iNodeNumFile, &iNodeEntryFile);

  if ((iNodeEntryFile.iNodeStat.st_mode & G_IFREG) != G_IFREG)
    return (-2);

  //on save combien y a de fichiers dans le dossier parent avant de lui faire diminuer sa size
  int nbFile = iNodeEntryDir.iNodeStat.st_size / sizeof(DirEntry);

  //on diminue sa size à lui meme - 1
  iNodeEntryDir.iNodeStat.st_size -= sizeof(DirEntry);

  //Contenu du dossier parent à transformer en dirEntry
  ReadBlock(iNodeEntryDir.Block[0], blockData);
  DirEntry *dirItems = (DirEntry *) blockData;
  int i = 0;
  int shift = 0;

  while (i < nbFile) {
      //si c'est le dossier a virer
    if (strcmp(dirItems[i].Filename, filename) == 0 || shift == 1) {
      //on active le décallage
      shift = 1;
      dirItems[i] = dirItems[i + 1];
    }
    i++;
  }
  //on update notre dirEntry
  WriteBlock(iNodeEntryDir.Block[0], blockData);

  iNodeEntryFile.iNodeStat.st_nlink--;
  updateInode(&iNodeEntryDir);
  updateInode(&iNodeEntryFile);

  // release block + inode
  if (iNodeEntryFile.iNodeStat.st_nlink == 0) {
    ReleaseFreeBlock(iNodeEntryFile.Block[0]);
    releaseFreeInode(iNodeNumFile);
  }

  return (0);
}

/*
  Cette fonction change la taille d’un fichier présent sur le disque. Pour les erreurs, la fonction retourne -1
  si le fichier pFilename est inexistant, -2 si pFilename est un répertoire. Autrement, la fonction retourne
  la nouvelle taille du fichier. Si NewSize est plus grand que la taille actuelle, ne faites rien et retournez la
  taille actuelle comme valeur. N’oubliez-pas de marquer comme libre les blocs libérés par cette
  fonction, si le changement de taille est tel que certains blocs sont devenus inutiles. Dans notre cas,
  ce sera si on tronque à la taille 0.

  Return:
    * NewSize or actual size on success
    * -1 if pFilename does not exist
    * -2 if pFilname is a directory

*/
int bd_truncate(const char *pFilename, int NewSize)
{
  ino iNodeNum;
  iNodeEntry iNodeEntryFile;

  iNodeNum = getInodeNumberFromPath(ROOT_INODE, pFilename);

  if (iNodeNum == -1 || iNodeNum == -2)
    return (-1);

  getInodeEntry(iNodeNum, &iNodeEntryFile);

  if ((iNodeEntryFile.iNodeStat.st_mode & G_IFDIR) == G_IFDIR)
    return (-2);

  if (NewSize > iNodeEntryFile.iNodeStat.st_size) {
    return (iNodeEntryFile.iNodeStat.st_size);
  }

  if (NewSize == 0) {
    ReleaseFreeBlock(iNodeEntryFile.Block[0]);
  }

  iNodeEntryFile.iNodeStat.st_size = NewSize;
  updateInode(&iNodeEntryFile);

  return (NewSize);
}

/*
Cette  fonction  sert  à  effacer  un  répertoire  vide,  i.e
.  s’il  ne  contient  pas  d’autre  chose  que  les  deux 
répertoires « . » et « .. ». Si le répertoire n’est
 pas vide, ne faites rien et retournez -3. N’oubliez-pas de 
décrémenter st_nlink pour le répertoire parent « .. ». Si le répertoire 
est inexistant, retourner -1. Si c’est 
un fichier régulier, retournez -2. Autrement, retournez 0 
pour indiquer le succès. 
*/
int bd_rmdir(const char *pFilename)
{
  char left[500];
  char right[FILENAME_SIZE];
  ino iNodeLeft;
  ino iNodeRight;
  iNodeEntry iNodeEntryLeft;
  iNodeEntry iNodeEntryRight;
  char blockLeft[BLOCK_SIZE];
  int blockNumLeft;

  //on découpe le path en left and right
  GetFilenameFromPath(pFilename, right);
  GetDirFromPath(pFilename, left);

  //on recupere leur numéro d'inode
  iNodeLeft = getInodeNumberFromPath(ROOT_INODE, left);
  iNodeRight = getInodeNumberFromPath(ROOT_INODE, pFilename);

  if (iNodeLeft == -1 || iNodeLeft == -2 || iNodeRight == -1 || iNodeRight == -2)
    {
      return (-1);
    }

  //on recupere leurs infos
  getInodeEntry(iNodeLeft, &iNodeEntryLeft);
  getInodeEntry(iNodeRight, &iNodeEntryRight);

  //on check si c'est un dossier qu'on nous demande de supprimer
  if (isFolder(iNodeEntryRight) != 1)
    {
        return (-2);
    }

  //on exit si le dossier est pas vide
  if(NumberofDirEntry(iNodeEntryRight.iNodeStat.st_size) > 2)
    {
      return (-3);
    }

  //maintenant qu'on est bon on diminune le compteur
  iNodeEntryLeft.iNodeStat.st_nlink--;

  //on save combien y a de fichiers dans le dossier parent avant de lui faire diminuer sa size
  int nbFile = iNodeEntryLeft.iNodeStat.st_size / sizeof(DirEntry);

  //on diminue sa size à lui meme - 1
  iNodeEntryLeft.iNodeStat.st_size -= sizeof(DirEntry);
  //on update
  updateInode(&iNodeEntryLeft);

  //Contenu du dossier parent à transformer en dirEntry
  blockNumLeft = iNodeEntryLeft.Block[0];
  ReadBlock(blockNumLeft, blockLeft);
  DirEntry * dirItems = (DirEntry *) blockLeft;
  int i = 0;
  int shift = 0;

  while (i < nbFile)
    {
      //si c'est le dossier a virer
      if (strcmp(dirItems[i].Filename, right) == 0 || shift == 1)
	{
	  //on active le décallage
	  shift = 1;
	  dirItems[i] = dirItems[i + 1];
	}
      i++;
    }
  //on update notre dirEntry
  WriteBlock(blockNumLeft, blockLeft);

  updateInode(&iNodeEntryLeft);
  releaseFreeInode(iNodeRight);
  ReleaseFreeBlock(iNodeEntryRight.Block[0]);

  return (0);
}

/*
Cette fonction sert à déplacer ou renommer un fichier ou répertoire  pFilename.  Le nom et le 
répertoire destination est le nom complet pFilenameDest. Par exemple, bd_rename("/tmp/a.txt","/doc/c.txt")
va  déplacer  le  fichier  de /tmp vers /doc,  et  aussi renommer le fichier de a.txt à c.txt.
N’oubliez-pas de retirer le fichier (ou répertoire) du répertoire 
initial. Aussi, faites attention à ne pas faire d’erreur si vous manipuler le compteur de lien. Si le fichier 
pFilename est un répertoire, n’oubliez-pas de mettre à jour le numéro d’i-node du répertoire parent « .. ». 
En cas de succès, retournez 0. Attention! Il se peut que le répertoire soit le même pour pFilename
et pFilenameDest. Votre programme doit pouvoir supporter cela, comme dans le cas bd_hardlink. Si 
le fichier pFilename est inexistant, ou si le répertoire de pFilenameDestest inexistant, retournez -1. 
Pour  vous  simplifier  la  vie,  vous  n’avez  pas  besoin  de gérer  le  cas  invalide  où  le  fichier  déplacé 
pFilename est un répertoire, et la destination 
pFilenameDest est un sous-répertoire de celui-ci. 
*/
int bd_rename(const char *pFilename, const char *pDestFilename)
{
  DirEntry * finalDirEntry;
  char srcLeft[BLOCK_SIZE];
  char srcRight[BLOCK_SIZE];
  char destLeft[BLOCK_SIZE];
  char destRight[BLOCK_SIZE];
  char blockData[BLOCK_SIZE];
  iNodeEntry srcLeftInodeEntry;
  iNodeEntry destLeftInodeEntry;
  iNodeEntry srcInodeEntry;
  ino srcIno;
  ino srcLeftIno;
  ino destIno;
  int ret;
  int blockNum;

  if (strcmp(pFilename, pDestFilename) == 0) {
    return (0);
  }

  ret = bd_hardlink(pFilename, pDestFilename);

  if (ret == -1 || ret == -2)//fail
    {
      return (-1);
    }
  else if (ret == 0)//fichier. Donc on peut le virer
    {
      bd_unlink(pFilename);
    }
  else {//c'est un repertoire


    //on découpe les morceaux dont on a besoin dans le path
    if (GetDirFromPath(pDestFilename, destLeft) == 0)
      return (-1);

    if (GetFilenameFromPath(pDestFilename, destRight) == 0)
      return (-1);

    if (GetDirFromPath(pFilename, srcLeft) == 0)
      return (-1);

    if (GetFilenameFromPath(pFilename, srcRight) == 0)
      return (-1);

    //récupération des numéros d'inodes du dossier source parent et du fichier source et du dossier source final
    if ((srcIno = getInodeNumberFromPath(ROOT_INODE, pFilename)) == -1)
      return (-1);

    if ((destIno = getInodeNumberFromPath(ROOT_INODE, destLeft)) == -1)
      return (-1);    
    
    if ((srcLeftIno = getInodeNumberFromPath(ROOT_INODE, srcLeft)) == -1)
      return (-1);


    //Recupération du contenu des inodes
    if (getInodeEntry(srcLeftIno, &srcLeftInodeEntry) != 0)
      return (-1);
    if (getInodeEntry(destIno, &destLeftInodeEntry) != 0)
      return (-1);
    if (getInodeEntry(srcIno, &srcInodeEntry) != 0)
      return (-1);
    //récupération du numéro de bloc src pour swapper plus bas
    blockNum = srcInodeEntry.Block[0];

    //on save combien y a de fichiers dans le dossier parent avant de lui faire diminuer sa size
    int nbFile = srcLeftInodeEntry.iNodeStat.st_size / sizeof (DirEntry);
    
    //on diminue sa size à lui meme - 1 (apres avoir save le nb de fichier dans nbFiles)
    srcLeftInodeEntry.iNodeStat.st_size -= sizeof(DirEntry);

    updateInode(&srcLeftInodeEntry);
    
    //Contenu du dossier parent à transformer en dirEntry
    int blockNumLeft = srcLeftInodeEntry.Block[0];
    char blockLeft[BLOCK_SIZE];
    ReadBlock(blockNumLeft, blockLeft);
    DirEntry * dirItems = (DirEntry *) blockLeft;
    int i = 0;
    int shift = 0;

    while (i < nbFile)
      {
	if (strcmp(dirItems[i].Filename, srcRight) == 0 || shift == 1) {
	  //on active le décallage
	  shift = 1;
	  dirItems[i] = dirItems[i + 1];
	}
	i++;
      }
    
    WriteBlock(blockNumLeft, blockLeft);

    // Décrémenter le nombre de link
    if (strcmp(srcLeft, destLeft) != 0)
      srcLeftInodeEntry.iNodeStat.st_nlink--;

    //on update le dossier parent src
    updateInode(&srcLeftInodeEntry);
    getInodeEntry(destIno, &destLeftInodeEntry);

    if (strcmp(srcLeft, destLeft) == 0)
      updateDir(&destLeftInodeEntry, srcIno, 0, destRight);
    else
      updateDir(&destLeftInodeEntry, srcIno, 1, destRight);
    
    updateInode(&destLeftInodeEntry);
    
    ReadBlock(blockNum, blockData);
    finalDirEntry = (DirEntry *) blockData;
    //on se déplace dans la nouvelle zone
    finalDirEntry++;
    //on fait le swap
    finalDirEntry->iNode = destIno;
    WriteBlock(blockNum, blockData);
    return (0);
  }
}


/*
Cette  fonction  est  utilisée  pour  permettre  la  lecture du  contenu  du  répertoire 
pDirLocation.  Si  le répertoire pDirLocation est valide, il faut allouer un tableau de 
DirEntry (via malloc) de la bonne taille et recopier le contenu du fichier répertoire dans 
ce tableau. Ce tableau est retourné à l’appelant via le double pointeur ppListeFichiers.
Par exemple, vous pouvez faire : (*ppListeFichiers) = (DirEntry*)malloc(taille_de_données);
et  traitez (*ppListeFichiers) comme  un  pointeur  sur  un  tableau  de DirEntry.  La  fonction 
bd_readdir retourne comme valeur le nombre de fichiers et sous-répertoires contenus dans ce répertoire 
pDirLocation (incluant . et ..). S’il y a une erreur, retourner -1. L’appelant sera en charge de désallouer 
la mémoire via free. 
*/
int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers)
{
  ino iNodeNum;
  if ((iNodeNum  = getInodeNumberFromPath(ROOT_INODE, pDirLocation)) < 0)
    return (-1);
  iNodeEntry iNodeEntry;
  char dataBlock[BLOCK_SIZE];

  getInodeEntry(iNodeNum, &iNodeEntry);

  if (isFolder(iNodeEntry) != 1)
    return (-1);
  
  ReadBlock(iNodeEntry.Block[0], dataBlock);

  *ppListeFichiers = (DirEntry*) malloc(iNodeEntry.iNodeStat.st_size);
  memcpy(*ppListeFichiers, dataBlock, iNodeEntry.iNodeStat.st_size);

  return NumberofDirEntry(iNodeEntry.iNodeStat.st_size);
}

/*
  Cette fonction est utilisée pour créer un lien symbolique vers pPathExistant. Vous devez ainsi créer
  un nouveau fichier pPathNouveauLien, en prenant soin que les drapeaux G_IFLNK et G_IFREG soient
  tous les deux à 1. La chaîne de caractère pPathExistant est simplement copiée intégralement dans le
  nouveau fichier créé (pensez à réutiliser bd_write ici). Ne pas vérifier la validité de pPathExistant.
  Assurez-vous que le répertoire qui va contenir le lien spécifié dans pPathNouveauLien existe, sinon
  retournez -1. Si le fichier pPathNouveauLien, existe déjà, retournez -2. Si tout se passe bien, retournez
  0. ATTENTION! Afin de vous simplifier la vie, si une commande est envoyée à votre système UFS sur
  un lien symbolique, ignorez ce fait. Ainsi, si /slnb.txt pointe vers b.txt et que vous recevez la commande
  ./ufs read /slnb.txt 0 40, cette lecture retournera b.txt et non pas le contenu de b.txt. Notez
  l’absence du caractère « / » au début de la chaîne de caractères. La commande suivante bd_readlink
  sera utilisée par le système d’exploitation pour faire le déréférencement du lien symbolique, plus tard
  quand nous allons le monter dans Linux avec FUSE

  Return:
    * 0 on success
    * -1 if pPathNouveauLien directory does not exist
    * -2 if pPathNouveauLien already exist
*/
int bd_symlink(const char *pPathExistant, const char *pPathNouveauLien)
{
  ino iNodeNumNew;
  iNodeEntry iNodeEntryNewFile;

  int ret = bd_create(pPathNouveauLien);

  if (ret == -1 || ret == -2)
    return (ret);

  iNodeNumNew = getInodeNumberFromPath(ROOT_INODE, pPathNouveauLien);
  getInodeEntry(iNodeNumNew, &iNodeEntryNewFile);

  iNodeEntryNewFile.iNodeStat.st_mode |= G_IFLNK;
  updateInode(&iNodeEntryNewFile);

  int numbytes = strlen(pPathExistant) + 1;

  bd_write(pPathNouveauLien, pPathExistant, 0, numbytes);

  return (0);
}

/*
  Cette fonction est utilisée pour copier le contenu d’un lien symbolique pPathLien, dans le buffer
  pBuffer de taille sizeBuffer. Ce contenu est une chaîne de caractère, sans NULL à la fin, représentant
  le path du fichier sur lequel ce lien symbolique pointe. Cette fonction permettra ainsi au système de fichier,
  une fois montée dans Linux, de déréférencer les liens symboliques. Si le fichier pPathLien n’existe pas
  ou qu’il n’est pas un lien symbolique, retournez -1. Sinon, retournez le nombre de caractères lus.
*/
int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer)
{
  int nb = 0;
  ino iNodeNum;
  iNodeEntry iNodeEntry;
  char blockData[BLOCK_SIZE];

  iNodeNum = getInodeNumberFromPath(ROOT_INODE, pPathLien);

  // Fichier inexsitant
  if (iNodeNum == -2)
    return -1;

  getInodeEntry(iNodeNum, &iNodeEntry);

  if ((iNodeEntry.iNodeStat.st_mode & G_IFLNK) != G_IFLNK)
    return -1;

  ReadBlock(iNodeEntry.Block[0], blockData);

  while (nb < iNodeEntry.iNodeStat.st_size && nb < sizeBuffer) {
    pBuffer[nb] = blockData[nb];
    ++nb;
  }

  return nb;
}

