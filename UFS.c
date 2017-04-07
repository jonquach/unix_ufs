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

int getFileINodeNumFromParent(const char *pFileName, int parentINodeNum);
int getInode(const char *pPath, const char *pFilename, int parentINodeNum);
int getFileINodeNumFromPath(const char *pPath);
int getINodeEntry(ino iNodeNum, iNodeEntry *pIE);

/* Retourne le numero d'inode du fichier pFileName dans le répertoire associé au numero d'inode parentINodeNum */
int getFileINodeNumFromParent(const char *pFileName, int parentINodeNum) {
	if (strcmp(pFileName, "") == 0)
	  return parentINodeNum;
	char blockData[BLOCK_SIZE];
	// On trouve le numero du block d'i-nodes qui contient le numero d'i-node parent
	printf("parentInodeNum = %d\n", parentINodeNum);
	int iNodesBlockNum = BASE_BLOCK_INODE + (parentINodeNum / NUM_INODE_PER_BLOCK);
	printf("iNodessBlockNum = %d\n", iNodesBlockNum);
	// Lecture du block d'i-nodes
	ReadBlock(iNodesBlockNum, blockData);
	iNodeEntry *pINodes = (iNodeEntry *) blockData;
	// On trouve la position de l'i-node parent dans le block d'i-nodes
	UINT16 iNodePosition = parentINodeNum % NUM_INODE_PER_BLOCK;
	// On trouve le nombre d'entrées dans le block de l'i-node parent
	UINT16 entryNum = NumberofDirEntry(pINodes[iNodePosition].iNodeStat.st_size);
	// Lecture du block de données associé à l'i-node parent
	ReadBlock(pINodes[iNodePosition].Block[0], blockData);
	DirEntry *pDE = (DirEntry *) blockData;
	// Pour chaque entrée du block (sauf . et ..) on vérifie le nom de fichier
	size_t n;
	for (n = 0; n < entryNum; n++) {
		if (strcmp(pFileName, pDE[n].Filename) == 0) {
			return pDE[n].iNode;	// On a trouvé le numéro d'i-node correspondant au nom de fichier/repertoire
		}
	}
	return -1;	// Le nom de fichier/répertoire n'existe pas
}



/* Fait une récursion sur le path pPath et retourne le numéro d'inode du fichier pFilename */
int getInode(const char *pPath, const char *pFilename, int parentINodeNum) {
	if (parentINodeNum == -1) return -1;

	char pName[FILENAME_SIZE];
	int iCar, iSlash = 0;
	for (iCar = 0; iCar < FILENAME_SIZE; iCar++) {
	  //printf("char = %c\n", pPath[iCar]);
	  if (pPath[iCar] == 0)
	    break;//arrivé au bout
	  else if (pPath[iCar] == '/' && iCar != 0)//on break si on tombe sur un / mais pas le premier (celui de root)
	    break;
	  else if (pPath[iCar] == '/')
	    iSlash++;
	  else {
	    pName[(iCar-iSlash)] = pPath[iCar];
	    //printf("a chaque tours = %s\n", pName);
	  }
	}
	pName[iCar - iSlash] = 0;//pour mettre un /0 je supose
	printf("pName de linfinie = %s\n", pName);
	if (strcmp(pFilename, pName) == 0) {//si mon nouveau pName c'est le meme que le fileName alors c bon. Sinon on recommence en passant au / d'après
		return getFileINodeNumFromParent(pName, parentINodeNum);
	} else {
		getInode(pPath + strlen(pName) + 1, pFilename, getFileINodeNumFromParent(pName, parentINodeNum));
	}
}

/* Retourne le numero d'inode correspondant au fichier spécifié par le path */
int getFileINodeNumFromPath(const char *pPath) {
  printf("path = %s\n", pPath);
  if (strcmp(pPath, "/") == 0)
    return ROOT_INODE;
  char pName[FILENAME_SIZE];
  if (GetFilenameFromPath(pPath, pName) == 0)
    pName[0] = 0;
  printf("pname chelou = %s\n", pName);
  return getInode(pPath, pName, ROOT_INODE);
}

/*Changer noms de variable et compagnie !!*/
/* Assigne un inodeEntry correspondant au numero d'inode iNodeNum au pointeur pIE */
int getINodeEntry(ino iNodeNum, iNodeEntry *pIE) {
  UINT16 iNodesBlockNum = -1;
  UINT16 iNodePosition = -1;
  
  printf("inode number = %d\n", iNodeNum);
  
  if (iNodeNum > N_INODE_ON_DISK || iNodeNum < 0)
    return -1;
  char blockData[BLOCK_SIZE];
  // On trouve le numero du block d'i-nodes qui contient le numero d'i-node
  //UINT16 iNodesBlockNum = BASE_BLOCK_INODE + (iNodeNum / NUM_INODE_PER_BLOCK);
  //printf("iNodesBlockNum = %d = %d + (%d / %d)\n", iNodesBlockNum, BASE_BLOCK_INODE, iNodeNum, NUM_INODE_PER_BLOCK);

  if (iNodeNum >= 0 && iNodeNum <= 15)
    {
      iNodesBlockNum = 4;
      iNodePosition = iNodeNum;
    }
  else if (iNodeNum >= 16 && iNodeNum <= 31)
    {
      iNodesBlockNum = 5;
      iNodePosition = NUM_INODE_PER_BLOCK - iNodeNum; //on fait la difference pour partir de 0 dans le bloc 2
    }
  else
    return (-1);

 
  
  // On trouve la position de l'i-node dans le block d'i-node
  //UINT16 iNodePosition = iNodeNum % NUM_INODE_PER_BLOCK;
  //printf("position dans le bloc = %d\n", iNodeNum % NUM_INODE_PER_BLOCK);

  
  
  // Lecture du block d'i-nodes
  ReadBlock(iNodesBlockNum, blockData);
  iNodeEntry *pINodes = (iNodeEntry *) blockData;
  *pIE = pINodes[iNodePosition];
  return 0;
}
/*
int getPathPart(char *path, int i, char *rPart)
{

}

void getInodeNumByName(char *path)
{
  if (strcmp(path, "/") == 0)
    return ROOT_INODE;
  
  int i = 0;
  char *currentPart;

  while ((i = getPathPart(path, i)) != 0)
    {
      
    }
}
*/
/* PPPAAAATTTTAAATTTEEEEE*/

void lolilol(const char *pFilename, char *buffer, int offset, int numbytes)
{
  UINT16 DirBlockNum = 6;
  char DataBlockDirEntry[BLOCK_SIZE];
  int iNodeNumber = 0;
  
  ReadBlock(DirBlockNum, DataBlockDirEntry);
  DirEntry *pDirEntry = (DirEntry *)DataBlockDirEntry;

  printf("mdr = %s\n", pDirEntry[5].Filename);

  iNodeEntry pIE;
  if (getINodeEntry(pDirEntry[5].iNode, &pIE) != 0)
    printf("BBBAAMMM");
  
  printiNode(pIE);

  //printf("contenu  (ou pas)= %d\n", pIE.iNodeStat.st_nlink);
  char fileDataBlock[BLOCK_SIZE];
  ReadBlock(pIE.Block[0], fileDataBlock);
  printf("contenu = %s\n", fileDataBlock);
  
  //if (strcmp("toto.txt",pDirEntry[0].Filename) == 0) {
  //  iNodeNumber = pDirEntry[0].iNode;
  //}
}

int isFolder(iNodeEntry iNodeStuff)
{
  if (iNodeStuff.iNodeStat.st_mode & G_IFDIR)
    return 1;
  else
    return 0;
}

char *getMostLeftPathPart(char *path)
{
  int i = 1;
  int ct = 1;

  if (strlen(path) <= 1)// genre si c'est que "/" ben je sais pas trop quoi faire
    {
      return path;
    }

  while (path[i] != '\0')
    {
      if (path[i] == '/')//on compte pas le 1er
	{
	  break;
	}
      ct++;
      i++;
    }

  char *newString = malloc(ct * sizeof(char));
  i = 1;
  while (path[i] != '\0')
    {
      newString[i - 1] = path[i];
      i++;
    }
  newString[i - 1] = '\0';
  //printf("newString = %s\n", newString);

  i = 0;
  while (path[i] != '\0')
    {
      path = path + 1;
      i++;
    }
  return newString;
}

//au debut ROOT_INODE
ino lol(ino inode, char *pathToFind)
{
  iNodeEntry pIE;
  char fileDataBlock[BLOCK_SIZE];

  getINodeEntry(inode, &pIE);
  ReadBlock(pIE.Block[0], fileDataBlock);
  printf("remaining path = %s\n", pathToFind);
  char *leftPathPart = getMostLeftPathPart(pathToFind);
  printf("newString = %s\n", leftPathPart);
  
  if (isFolder(pIE) == 1)
    {
      printf("folder\n");
      DirEntry *pDirEntry = (DirEntry *)fileDataBlock;
      int nbFile = pIE.iNodeStat.st_size / sizeof(DirEntry);
      printf("nbFIle = %d\n", nbFile);
      int i = 0;
      while(i < nbFile)
	{
	  printf("current name = %s\n", pDirEntry[i].Filename);
	  if (strcmp(pDirEntry[i].Filename, leftPathPart) == 0)//si le nom de dossier est le meme que le nom de path suivant alors on peut choper son inode et recommencer
	    {
	      printf("match\n");
     	      return lol(pDirEntry[i].iNode, pathToFind); //dans la structure y a l'inode a coté du filename
	    }
	  i++;
	}
    }
  else
    {
      return pIE.iNodeStat.st_ino;
      printf("file = %s\n", pathToFind);
    }
  
}

/*MMMMEEEESSSSS  TTTTTEEEESSSSSTTTTTT*/

int bd_countfreeblocks(void) {
	return 0;
}

int bd_stat(const char *pFilename, gstat *pStat) {
	return -1;
}

int bd_create(const char *pFilename) {
	return -1;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {

  ino inodeNum = lol(ROOT_INODE, pFilename);
  iNodeEntry pIE;

  getINodeEntry(inodeNum, &pIE);
  char fileData[BLOCK_SIZE];
  ReadBlock(pIE.Block[0], fileData);
  printf("file stuff shit = %s\n", fileData);

  int ctRead = 0;
  int i = offset;
  //while (ctRead < numbytes && i < pIE.iNodeStat.st_size)
  while (i < (offset + numbytes) && i < pIE.iNodeStat.st_size)
    {
      buffer[ctRead] = fileData[i];
      ctRead++;
      i++;
    }
  printf("i = %d\n", i);
  buffer[ctRead] = '\0';
  printf("while or not while that is the question = %s (taille = [%d])\n", buffer, strlen(buffer));

  return ctRead;
  /*  ino iNodeNum = getFileINodeNumFromPath(pFilename);
  iNodeEntry iNode;

  if (iNodeNum == -1) return -1;							// Le fichier pFilename est inexistant
  if (getINodeEntry(iNodeNum, &iNode) != 0) return -1;	// Le fichier pFilename est inexistant
  if (iNode.iNodeStat.st_mode & G_IFDIR) return -2; 		// Le fichier pFilename est un répertoire
  if (iNode.iNodeStat.st_size <= offset) return 0; 		// L'offset engendre un overflow
  
  char fileDataBlock[BLOCK_SIZE];
  ReadBlock(iNode.Block[0], fileDataBlock);
  int i = 0, octets = 0;
  for (i = offset; i < iNode.iNodeStat.st_size && i < (offset + numbytes); i++) {
    buffer[octets] = fileDataBlock[i];
    octets++;
  }
  return octets; // retourne le nombre d'octets lus
  return -1;*/
}

int bd_mkdir(const char *pDirName) {
	return -1;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) { 
	return -1;
}

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
	return -1;
}

int bd_unlink(const char *pFilename) {
	return -1;
}

int bd_truncate(const char *pFilename, int NewSize) {
	return -1;
}

int bd_rmdir(const char *pFilename) {
	return -1;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
	return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
	return -1;
}

int bd_symlink(const char *pPathExistant, const char *pPathNouveauLien) {
    return -1;
}

int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer) {
    return -1;
}

