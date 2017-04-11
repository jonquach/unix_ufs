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
int getFreeBlock();
void updateInode(iNodeEntry *ine);
void updateDir(iNodeEntry * destDirInode, ino inodeNum, int inc, char *filename);

int getInodeBlockNumAndPos(ino iNodeNum, int *iNodeBlockNum, int *iNodePosition) {
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

  printf("iNodeNum = %d\n", iNodeNum);
  printf("iNodeBlockNum = %d\n", *iNodeBlockNum);
  printf("iNodePosition = %d\n", *iNodePosition);

  return 0;
}

/*Recupere le inodeEntry en utilisant un numéro d'inode*/
int getInodeEntry(ino iNodeNum, iNodeEntry *inodeEntry)
{
  int iNodeBlockNum = -1;
  int iNodePosition = -1;
  iNodeEntry *inodesBlock = NULL;
  char blockData[BLOCK_SIZE];
  
  // printf("inode number = %d\n", iNodeNum);
  
  if (iNodeNum > N_INODE_ON_DISK || iNodeNum < 0)
    return -1;

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

int isFolder(iNodeEntry iNodeStuff)
{
  if (iNodeStuff.iNodeStat.st_mode & G_IFDIR)
    return 1;
  else
    return 0;
}

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
      path++;//on incremente le pointeur du coup on bouf la chaine petit a petit (et apres avoir fait la copie du char dans newString)
      i++;
    }
  newString[i] = '\0';
  *leftPart = newString;
  return (i + 1);
}

//au debut ROOT_INODE
ino getInodeNumberFromPath(ino inode, const char *pathToFind)
{
  iNodeEntry iNodeEntry;
  char fileDataBlock[BLOCK_SIZE];

  if (strcmp(pathToFind, "/") == 0)
    return ROOT_INODE;
  getInodeEntry(inode, &iNodeEntry);
  // printf("remaining path = %s\n", pathToFind);
    
  if (isFolder(iNodeEntry) == 1)
    {
      //printf("folder\n");

      char *leftPathPart = NULL;
      int ret = getLeftPart(pathToFind, &leftPathPart);
      
      
      if (ret == -1) //on est arrivé au bout du path.
	{
	  // printf("On termine sur un dossier !!\n");
	  return iNodeEntry.iNodeStat.st_ino;
	}
      
      pathToFind += ret;//on increment le ptr ici car dans getLeftPart ca fonctionne pas... 
      
      //printf("newString = %s\n", leftPathPart);
      ReadBlock(iNodeEntry.Block[0], fileDataBlock);
      DirEntry *dirEntry = (DirEntry *)fileDataBlock;
      int nbFile = iNodeEntry.iNodeStat.st_size / sizeof(DirEntry);
      //printf("nbFIle = %d\n", nbFile);
      int i = 0;
      while(i < nbFile)
	{
	  //printf("current name = %s\n", pDirEntry[i].Filename);
	  if (strcmp(dirEntry[i].Filename, leftPathPart) == 0)//si le nom de dossier est le meme que le nom de path suivant alors on peut choper son inode et recommencer
	    {
	      //printf("match\n");
     	      return getInodeNumberFromPath(dirEntry[i].iNode, pathToFind); //dans la structure y a l'inode a coté du filename
	    }
	  i++;
	}
      // NOT FOUND
    // printf("pathToFind %d\n", strlen(pathToFind));
    // printf("pathToFind %s\n", pathToFind);
    if (strlen(pathToFind) != 0) {
      // Folder does not exist
      return -1;
    }
    // File does not exist
    return -2;
    }
  else
    {
      //printf("file = %s\n", pathToFind);
      return iNodeEntry.iNodeStat.st_ino;
    }
}

int findFirstFreeInode(char freeInodes[BLOCK_SIZE])
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

int getFreeInode()
{
  char freeInodesBlock[BLOCK_SIZE];
  int freeInode = -1;
  
  ReadBlock(FREE_INODE_BITMAP, freeInodesBlock);
  freeInode = findFirstFreeInode(freeInodesBlock);
  
  if (freeInode != -1) {
    freeInodesBlock[freeInode] = 0;
    printf("GLOFS: Saisie i-node %d\n", freeInode); //check si c le bon message
    WriteBlock(FREE_INODE_BITMAP, freeInodesBlock);
    return freeInode;
  }
  return -1;
}

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

int releaseFreeInode(unsigned int inodeNumber)
{
  char blockData[BLOCK_SIZE];

  ReadBlock(FREE_INODE_BITMAP, blockData);
  blockData[inodeNumber] = 1;
  
  WriteBlock(FREE_INODE_BITMAP, blockData);
  printf("GLOFS: Relache i-node %d\n", inodeNumber); //Check si c la bonne phrase
  return (1);
}


int ReleaseFreeBlock(UINT16 BlockNum)
{
  char BlockFreeBitmap[BLOCK_SIZE];
  
  ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
  BlockFreeBitmap[BlockNum] = 1;
  printf("GLOFS: Relache bloc %d\n", BlockNum);
  WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap);
  return (1);
} 

int bd_countfreeblocks(void) {
  int nbFreeBlocks;
  char freeBlock[BLOCK_SIZE];

  ReadBlock(FREE_BLOCK_BITMAP, freeBlock);

  for (int i = 0; i < N_BLOCK_ON_DISK; ++i) {
    if (freeBlock[i] != 0)
      nbFreeBlocks++;
  }

  return nbFreeBlocks;
}

int bd_stat(const char *pFilename, gstat *pStat)
{
  ino iNodeNum;
  iNodeEntry iNodeEntry;

  iNodeNum = getInodeNumberFromPath(ROOT_INODE, pFilename);

  if (iNodeNum == -2 || iNodeNum == -1)
    return -1;

  getInodeEntry(iNodeNum, &iNodeEntry);
  // pStat->st_ino = iNodeEntry.iNodeStat.st_ino;
  // // pStat->st_mode = iNodeEntry.iNodeStat.st_mode;
  // // pStat->st_nlink = iNodeEntry.iNodeStat.st_nlink;
  // // pStat->st_size = iNodeEntry.iNodeStat.st_size;
  // // pStat->st_blocks = iNodeEntry.iNodeStat.st_blocks;
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

  // printf("INDOE NUM %d\n", iNodeNum);

  iNodeNum = getFreeInode();

  // printf("INDOE NUM %d\n", iNodeNum);

  getInodeEntry(iNodeNum, &iNodeEntry);
  iNodeEntry.iNodeStat.st_ino = iNodeNum;
  iNodeEntry.iNodeStat.st_mode = G_IFREG | G_IRWXU | G_IRWXG;
  iNodeEntry.iNodeStat.st_nlink = 1;
  iNodeEntry.iNodeStat.st_size = 0;
  iNodeEntry.iNodeStat.st_blocks = 0;

  dirInode = getInodeNumberFromPath(ROOT_INODE, dirname);

  getInodeEntry(dirInode, &iNodeDir);

  // printf("iNodeDir.iNodeStat.st_ino --> %d\n", iNodeDir.iNodeStat.st_ino);
  // printf("iNodeEntry.iNodeStat.st_ino --> %d\n", iNodeEntry.iNodeStat.st_ino);

  // addDirEntryInDir(&iNodeDir, fileInode, strFile);
  updateDir(&iNodeDir, iNodeEntry.iNodeStat.st_ino, 0, filename);
  updateInode(&iNodeEntry);

  return 0;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
  ino inodeNum = getInodeNumberFromPath(ROOT_INODE, pFilename);
  iNodeEntry iNodeEntry;
  char fileData[BLOCK_SIZE];
  int ctRead = 0;
  int i = offset;

  getInodeEntry(inodeNum, &iNodeEntry);
  
  ReadBlock(iNodeEntry.Block[0], fileData);

  while (i < (offset + numbytes) && i < iNodeEntry.iNodeStat.st_size)
    {
      buffer[ctRead] = fileData[i];
      ctRead++;
      i++;
    }
  buffer[ctRead] = '\0';
  printf("while or not while that is the question = %s (taille = [%lu])\n", buffer, strlen(buffer));

  return ctRead;
}

int bd_mkdir(const char *pDirName)
{
  char pathRight[FILENAME_SIZE]; //////max ?? Test dans rmdir. Faudra pe etre changé ici si ca se passe bien
  char pathLeft[FILENAME_SIZE];
  ino iNodeNumLeft;
  iNodeEntry iNodeEntryLeft;
  ino iNodeNumRight;
  iNodeEntry iNodeEntryRight;
  
  //Découpage du path en left et right
  if (GetDirFromPath(pDirName, pathLeft) == 0)
    return (-1);
  if (GetFilenameFromPath(pDirName, pathRight) == 0)
    return (-1);

  //Récupération du numéro d'inode de left
  if ((iNodeNumLeft = getInodeNumberFromPath(ROOT_INODE, pathLeft)) == -1)
    return (-1);//pas sur que ca soit -1
  //Récupération de l'entry pour left
  getInodeEntry(iNodeNumLeft, &iNodeEntryLeft);

  //check que left est bien un dossier !
  if (isFolder(iNodeEntryLeft) != 1)//LNK aussi ?
    return -1;

  //on récupère un numéro d'inode libre (pour right)
  ino freeInodeNum = getFreeInode();
  int freeBlockNum = getFreeBlock();
  iNodeEntry freeInodeEntry;

  //maj du dossier parent
  updateDir(&iNodeEntryLeft, freeInodeNum, 1, pathRight);

  printf("not chez moi !\n");
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

  //printf("direntry filename = %s\n", dirEntry->Filename);
  //printf("direntry inode = %s\n", dirEntry->iNode);

  //et on le save
  WriteBlock(destDirInode->Block[0], dataBlock);
}

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

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) {
	return -1;
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

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
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

  if (iNodeNumExist == -1) return -1; // pPathExistant directory does not exist
  if (iNodeNumExist == -2) return -1; // pPathExistant file does not exist
  if (iNodeNumNewFile == -1) return -1; // pPathNouveauLien directory does not exist
  if (iNodeNumNewFile != -2) return -2; // pPathNouveauLien file already exist

  getInodeEntry(iNodeNumExist, &iNodeEntryExist);

  if ((iNodeEntryExist.iNodeStat.st_mode & G_IFDIR) == G_IFDIR) return -3;

  GetDirFromPath(pPathNouveauLien, dirname);
  iNodeNumDir = getInodeNumberFromPath(ROOT_INODE, dirname);

  getInodeEntry(iNodeNumDir, &iNodeDir);

  GetFilenameFromPath(pPathNouveauLien, fhardlink);

  updateDir(&iNodeDir, iNodeEntryExist.iNodeStat.st_ino, 1, fhardlink);
  iNodeEntryExist.iNodeStat.st_nlink++;
  updateInode(&iNodeEntryExist);

  return 0;
}

int bd_unlink(const char *pFilename) {
	return -1;
}

int bd_truncate(const char *pFilename, int NewSize) {
	return -1;
}

int bd_rmdir(const char *pFilename) {
  char *left;
  char right[FILENAME_SIZE];
  ino iNodeLeft;
  ino iNodeRight;
  iNodeEntry iNodeEntryLeft;
  iNodeEntry iNodeEntryRight;
  char blockLeft[BLOCK_SIZE];

  //on découpe le path en left and right
  GetFilenameFromPath(pFilename, right);
  GetDirFromPath(pFilename, left);

  //on recupere leur numéro d'inode
  iNodeLeft = getInodeNumberFromPath(ROOT_INODE, left);
  iNodeRight = getInodeNumberFromPath(ROOT_INODE, pFilename);

  //on recupere leurs infos
  getInodeEntry(iNodeLeft, &iNodeEntryLeft);
  getInodeEntry(iNodeRight, &iNodeEntryRight);

  //on check si c'est un dossier
  if (isFolder(iNodeEntryRight) != 1)
    return (-2);
  
  //on exit si le dossier est pas vide
  if(NumberofDirEntry(iNodeEntryRight.iNodeStat.st_size) > 2)
    return (-3);

  //maintenant qu'on est bon on diminune le compteur
  iNodeEntryLeft.iNodeStat.st_nlink--;


  //unsigned int copySizeLeft = inodeEntryLeft->iNodeStat.st_size; //ca ca sert à rien ????

  //on diminue sa size à lui meme - 1
  iNodeEntryLeft.iNodeStat.st_size -= sizeof(DirEntry);
  //on update...
  updateInode(&iNodeEntryLeft);

  int blockNum = iNodeEntryLeft.Block[0];
  
  ReadBlock(blockNum, blockLeft);
  
  DirEntry * dirItems = (DirEntry *) blockLeft;
  
  int i = 0;
  int shift = 1;
  int nbFile = iNodeEntryLeft.iNodeStat.st_size / sizeof(DirEntry);
  
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
  WriteBlock(blockNum, blockLeft);

  
  updateInode(&iNodeEntryLeft);
  releaseFreeInode(iNodeRight);
  ReleaseFreeBlock(iNodeEntryRight.Block[0]);

  return (0);
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
  return (-1);
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
  ino iNodeNum = getInodeNumberFromPath(ROOT_INODE, pDirLocation);
  iNodeEntry iNodeEntry;
  char dataBlock[BLOCK_SIZE];
  
  getInodeEntry(iNodeNum, &iNodeEntry);

  //if (iNodeNum == -1) return -1;// Le fichier pDirLocation est inexistant
  //if (getINodeEntry(iNodeNum, &iNode) != 0)  return -1; // Le fichier pDirLocation est inexistant
  //if (!(iNode.iNodeStat.st_mode & G_IFDIR)) return -1; // Le fichier pDirLocation n'est pas un répertoire

  ReadBlock(iNodeEntry.Block[0], dataBlock);

  *ppListeFichiers = (DirEntry*) malloc(iNodeEntry.iNodeStat.st_size);
  memcpy(*ppListeFichiers, dataBlock, iNodeEntry.iNodeStat.st_size);

  return NumberofDirEntry(iNodeEntry.iNodeStat.st_size);
}

int bd_symlink(const char *pPathExistant, const char *pPathNouveauLien) {
    return -1;
}

/*
  Cette fonction est utilisée pour copier le contenu d’un lien symbolique pPathLien, dans le buffer
  pBuffer de taille sizeBuffer. Ce contenu est une chaîne de caractère, sans NULL à la fin, représentant
  le path du fichier sur lequel ce lien symbolique pointe. Cette fonction permettra ainsi au système de fichier,
  une fois montée dans Linux, de déréférencer les liens symboliques. Si le fichier pPathLien n’existe pas
  ou qu’il n’est pas un lien symbolique, retournez -1. Sinon, retournez le nombre de caractères lus.
*/
int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer) {
  int nb = 0;
  ino iNodeNum;
  iNodeEntry iNodeEntry;
  char blockData[BLOCK_SIZE];

  iNodeNum = getInodeNumberFromPath(ROOT_INODE, pPathLien);

  // Fichier inexsitant
  if (iNodeNum == -2)
    return -1;

  getInodeEntry(iNodeNum, &iNodeEntry);

  // printf("%d\n%d\n", iNodeEntry.iNodeStat.st_mode & G_IFLNK, G_IFLNK);

  if ((iNodeEntry.iNodeStat.st_mode & G_IFLNK) != G_IFLNK)
    return -1;

  ReadBlock(iNodeEntry.Block[0], blockData);

  while (nb < iNodeEntry.iNodeStat.st_size && nb < sizeBuffer) {
    pBuffer[nb] = blockData[nb];
    ++nb;
  }

  return nb;
}

