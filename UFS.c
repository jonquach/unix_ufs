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
int getInodeEntry(ino iNodeNum, iNodeEntry *inodeEntry);
int isFolder(iNodeEntry iNodeStuff);
int getLeftPart(char *path, char **leftPart);
ino getInodeNumberFromPath(ino inode, char *pathToFind);
int findFirstFreeInode(char freeInodes[BLOCK_SIZE]);
int getFreeInode();


/*Recupere le inodeEntry en utilisant un numéro d'inode*/
int getInodeEntry(ino iNodeNum, iNodeEntry *inodeEntry)
{
  int iNodeBlockNum = -1;
  int iNodePosition = -1;
  iNodeEntry *inodesBlock = NULL;
  char blockData[BLOCK_SIZE];
  
  //printf("inode number = %d\n", iNodeNum);
  
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

int getLeftPart(char *path, char **leftPart)
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
ino getInodeNumberFromPath(ino inode, char *pathToFind)
{
  iNodeEntry iNodeEntry;
  char fileDataBlock[BLOCK_SIZE];
  
  getInodeEntry(inode, &iNodeEntry);
  ReadBlock(iNodeEntry.Block[0], fileDataBlock);
  //printf("remaining path = %s\n", pathToFind);
    
  if (isFolder(iNodeEntry) == 1)
    {
      //printf("folder\n");

      char *leftPathPart = NULL;
      int ret = getLeftPart(pathToFind, &leftPathPart);
      
      
      if (ret == -1) //on est arrivé au bout du path.
	{
	  printf("On termine sur un dossier !!\n");
	  return iNodeEntry.iNodeStat.st_ino;
	}
      
      pathToFind += ret;//on increment le ptr ici car dans getLeftPart ca fonctionne pas... 
      
      //printf("newString = %s\n", leftPathPart);
      DirEntry *pDirEntry = (DirEntry *)fileDataBlock;
      int nbFile = iNodeEntry.iNodeStat.st_size / sizeof(DirEntry);
      //printf("nbFIle = %d\n", nbFile);
      int i = 0;
      while(i < nbFile)
	{
	  //printf("current name = %s\n", pDirEntry[i].Filename);
	  if (strcmp(pDirEntry[i].Filename, leftPathPart) == 0)//si le nom de dossier est le meme que le nom de path suivant alors on peut choper son inode et recommencer
	    {
	      //printf("match\n");
     	      return getInodeNumberFromPath(pDirEntry[i].iNode, pathToFind); //dans la structure y a l'inode a coté du filename
	    }
	  i++;
	}
      return -1;
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
  printf("while or not while that is the question = %s (taille = [%d])\n", buffer, strlen(buffer));

  return ctRead;
}

int bd_mkdir(const char *pDirName) {
  printf("mais nike ta mere\n");
  char pathRight[FILENAME_SIZE];
  char pathLeft[FILENAME_SIZE];
  ino inodeNumLeft;
  iNodeEntry inodeEntryLeft;
  ino inodeNumRight;
  iNodeEntry inodeEntryRight;

int freeInode = getFreeInode();
 freeInode = getFreeInode();
  printf("lolilol = %d\n", freeInode);
  
  
  if (GetDirFromPath(pDirName, pathRight) == 0)
    return (-1);
  
  if (GetFilenameFromPath(pDirName, pathLeft) == 0)
    return (-1);

  if ((inodeNumLeft = getInodeNumberFromPath(ROOT_INODE, pathLeft)) == -1)
    return (-1);//pas sur que ca soit -1
  getInodeEntry(inodeNumLeft, &inodeEntryLeft);

  if (isFolder(inodeEntryLeft) != 1)//LNK aussi ?
    return -1;

  
  
  ino newInodeNumRight;
  iNodeEntry newInodeEntryRight;
  /*
  inodeEntryLeft.iNodeStat.st_nlink++;
  WriteBlock(FREE_INODE_BLOCK, inodeEntryLeft);
  
  
  
  */

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

int bd_readlink(const char *pPathLien, char *pBuffer, int sizeBuffer) {
    return -1;
}

